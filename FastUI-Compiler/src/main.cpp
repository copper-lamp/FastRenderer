#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <pugixml.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// ─── Attribute Name Mapping ───

static std::map<std::string, std::map<std::string, std::string>> g_attrMap = {
    {"button",   {{"label", "text"}, {"on_click", "onClick"}, {"bg_color", "color"}, {"text_color", "color"}}},
    {"window",   {{"label", "title"}, {"on_close", "onClose"}}},
    {"checkbox", {{"on_change", "onChange"}}},
    {"slider",   {{"on_change", "onChange"}, {"min_val", "min"}, {"max_val", "max"}}},
    {"input",    {{"hint", "placeholder"}}},
    {"combo",    {{"options", "options"}}},
    {"canvas",   {{"draw", "drawCommands"}}},
};

// ─── Expression Parsing ───

struct ExprInfo {
    bool hasExpr = false;
    std::string staticPrefix;
    std::string varRef;
    std::string staticSuffix;
};

static ExprInfo parseExpression(const std::string& raw) {
    ExprInfo info;
    size_t start = raw.find('{');
    if (start == std::string::npos) {
        info.staticPrefix = raw;
        return info;
    }
    size_t end = raw.find('}', start);
    if (end == std::string::npos) {
        info.staticPrefix = raw;
        return info;
    }
    info.hasExpr = true;
    info.staticPrefix = raw.substr(0, start);
    info.varRef = raw.substr(start + 1, end - start - 1);
    info.staticSuffix = raw.substr(end + 1);
    return info;
}

// ─── Style Definitions ───

struct StyleDef {
    std::string id;
    std::map<std::string, std::string> properties;
};

struct DefaultStyleDef {
    std::string type;
    std::map<std::string, std::string> properties;
};

// ─── Compiler State ───

struct CompilerState {
    std::map<std::string, pugi::xml_document> templates;
    std::vector<StyleDef> namedStyles;
    std::vector<DefaultStyleDef> defaultStyles;
    json resources = json::object();
    json scripts = json::object();
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void error(const std::string& msg) { errors.push_back(msg); }
    void warn(const std::string& msg) { warnings.push_back(msg); }
};

// ─── Load Templates ───

static void loadTemplates(CompilerState& state, const fs::path& templatesDir) {
    if (!fs::exists(templatesDir)) return;
    for (auto& entry : fs::directory_iterator(templatesDir)) {
        if (entry.path().extension() != ".xml") continue;
        pugi::xml_document doc;
        if (!doc.load_file(entry.path().string().c_str())) {
            state.warn("Failed to load template: " + entry.path().filename().string());
            continue;
        }
        auto root = doc.child("template");
        std::string id = root.attribute("id").value();
        if (!id.empty()) {
            state.templates[id] = std::move(doc);
        }
    }
}

// ─── Load Styles ───

static void loadStyles(CompilerState& state, const fs::path& styleFile) {
    if (!fs::exists(styleFile)) return;
    pugi::xml_document doc;
    if (!doc.load_file(styleFile.string().c_str())) {
        state.warn("Failed to load styles file");
        return;
    }
    auto root = doc.child("styles");
    if (!root) return;

    for (auto& node : root.children()) {
        std::string name = node.name();
        if (name == "style") {
            StyleDef def;
            def.id = node.attribute("id").value();
            for (auto& prop : node.children("property")) {
                def.properties[prop.attribute("name").value()] = prop.attribute("value").value();
            }
            state.namedStyles.push_back(def);
        } else if (name == "default") {
            DefaultStyleDef def;
            def.type = node.attribute("type").value();
            for (auto& prop : node.children("property")) {
                def.properties[prop.attribute("name").value()] = prop.attribute("value").value();
            }
            state.defaultStyles.push_back(def);
        }
    }
}

// ─── Load Resources ───

static void loadResources(CompilerState& state, const fs::path& resFile) {
    if (!fs::exists(resFile)) return;
    pugi::xml_document doc;
    if (!doc.load_file(resFile.string().c_str())) {
        state.warn("Failed to load resources file");
        return;
    }
    auto root = doc.child("resources");
    if (!root) return;
    for (auto& node : root.children()) {
        json item = json::object();
        for (auto& attr : node.attributes()) {
            item[attr.name()] = attr.value();
        }
        std::string id = node.attribute("id").value();
        if (!id.empty()) {
            state.resources[node.name()][id] = item;
        }
    }
}

// ─── Load Scripts ───

static void loadScripts(CompilerState& state, const fs::path& scriptFile) {
    if (!fs::exists(scriptFile)) return;
    pugi::xml_document doc;
    if (!doc.load_file(scriptFile.string().c_str())) {
        state.warn("Failed to load scripts file");
        return;
    }
    auto root = doc.child("scripts");
    if (!root) return;
    for (auto& node : root.children()) {
        std::string id = node.attribute("id").value();
        if (!id.empty()) {
            std::ostringstream ss;
            node.print(ss, "", pugi::format_raw);
            state.scripts[id] = ss.str();
        }
    }
}

// ─── Apply Default Styles ───

static void applyDefaultStyle(CompilerState& state, const std::string& type,
    std::map<std::string, std::string>& props)
{
    for (auto& def : state.defaultStyles) {
        if (def.type == type) {
            for (auto& [k, v] : def.properties) {
                if (props.find(k) == props.end()) {
                    props[k] = v;
                }
            }
        }
    }
}

// ─── Apply Named Style ───

static void applyNamedStyle(CompilerState& state, const std::string& styleId,
    std::map<std::string, std::string>& props)
{
    for (auto& def : state.namedStyles) {
        if (def.id == styleId) {
            for (auto& [k, v] : def.properties) {
                if (props.find(k) == props.end()) {
                    props[k] = v;
                }
            }
        }
    }
}

// ─── Convert XML Attribute Name to JSON Prop Name ───

static std::string mapAttr(const std::string& type, const std::string& attrName) {
    if (g_attrMap.count(type)) {
        auto it = g_attrMap[type].find(attrName);
        if (it != g_attrMap[type].end()) return it->second;
    }
    return attrName;
}

// ─── Forward Declaration ───

static json convertNode(CompilerState& state, const pugi::xml_node& xmlNode,
    const std::map<std::string, std::string>& parentLoopVars = {});

// ─── Process <use> Template Expansion ───

static json expandTemplate(CompilerState& state, const pugi::xml_node& useNode,
    const std::map<std::string, std::string>& parentLoopVars)
{
    std::string templateId = useNode.attribute("id").value();
    auto it = state.templates.find(templateId);
    if (it == state.templates.end()) {
        state.error("Template not found: " + templateId);
        json err;
        err["type"] = "text";
        err["props"]["value"] = "[Template '" + templateId + "' not found]";
        return err;
    }

    auto tplRoot = it->second.child("template");
    auto paramsAttr = tplRoot.attribute("params");
    std::vector<std::string> paramNames;

    // Collect template parameters
    if (paramsAttr) {
        std::string params = paramsAttr.value();
        size_t start = 0, end;
        while ((end = params.find(',', start)) != std::string::npos) {
            std::string p = params.substr(start, end - start);
            p.erase(0, p.find_first_not_of(" \t"));
            p.erase(p.find_last_not_of(" \t") + 1);
            if (!p.empty()) paramNames.push_back(p);
            start = end + 1;
        }
        std::string p = params.substr(start);
        p.erase(0, p.find_first_not_of(" \t"));
        p.erase(p.find_last_not_of(" \t") + 1);
        if (!p.empty()) paramNames.push_back(p);
    }

    // Build substitution map from <use> attributes
    std::map<std::string, std::string> subs = parentLoopVars;
    for (auto& attr : useNode.attributes()) {
        std::string attrName = attr.name();
        if (attrName == "id") continue;
        subs["{" + attrName + "}"] = attr.value();
    }

    // Also map explicitly to {param_name}
    for (auto& pn : paramNames) {
        std::string val = useNode.attribute(pn.c_str()).value();
        if (!val.empty()) {
            subs["{" + pn + "}"] = val;
        }
    }

    // Find the root child element of the template (skip <template> tag itself)
    pugi::xml_node tplContent = tplRoot.first_child();
    // Build a clone
    std::ostringstream xmlBuf;
    tplContent.print(xmlBuf, "", pugi::format_raw);
    std::string xmlStr = xmlBuf.str();

    // Apply substitutions for {param} placeholders in the XML string
    for (auto& [key, val] : subs) {
        size_t pos = 0;
        while ((pos = xmlStr.find(key, pos)) != std::string::npos) {
            xmlStr.replace(pos, key.length(), val);
            pos += val.length();
        }
    }

    // Re-parse the substituted XML
    pugi::xml_document substDoc;
    if (!substDoc.load_string(xmlStr.c_str())) {
        state.error("Template substitution parse failed for: " + templateId);
        json err;
        err["type"] = "text";
        err["props"]["value"] = "[Template error]";
        return err;
    }

    return convertNode(state, substDoc.first_child(), subs);
}

// ─── Convert a single XML node to JSON ───

static json convertNode(CompilerState& state, const pugi::xml_node& xmlNode,
    const std::map<std::string, std::string>& parentLoopVars)
{
    json node = json::object();
    std::string elemName = xmlNode.name();

    // Handle <use> template references
    if (elemName == "use") {
        return expandTemplate(state, xmlNode, parentLoopVars);
    }

    node["type"] = elemName;
    node["props"] = json::object();

    // Collect attributes
    std::map<std::string, std::string> props;
    for (auto& attr : xmlNode.attributes()) {
        std::string attrName = attr.name();
        props[mapAttr(elemName, attrName)] = attr.value();
    }

    // Apply style references
    if (props.count("style")) {
        applyNamedStyle(state, props["style"], props);
    }

    // Apply default styles
    applyDefaultStyle(state, elemName, props);

    // Handle v_if / v_elseif / v_else (store as props for renderer)
    // These are special: they modify child visibility, not props
    for (auto& [k, v] : props) {
        node["props"][k] = v;
    }

    // Process children
    json children = json::array();
    bool hasCond = false;
    std::string condVar;
    pugi::xml_node condNode;

    // Check for v_for
    std::string vForStr = xmlNode.attribute("v_for").value();
    if (!vForStr.empty()) {
        json forInfo = json::object();
        std::string forRaw = vForStr;
        // Parse "item in collection" or "(item, idx) in collection"
        size_t inPos = forRaw.find(" in ");
        if (inPos != std::string::npos) {
            std::string left = forRaw.substr(0, inPos);
            std::string collection = forRaw.substr(inPos + 4);
            // Remove whitespace
            left.erase(0, left.find_first_not_of(" \t"));
            left.erase(left.find_last_not_of(" \t") + 1);
            collection.erase(0, collection.find_first_not_of(" \t"));
            collection.erase(collection.find_last_not_of(" \t") + 1);

            if (left.front() == '(' && left.back() == ')') {
                std::string inner = left.substr(1, left.size() - 2);
                size_t commaPos = inner.find(',');
                if (commaPos != std::string::npos) {
                    forInfo["item"] = inner.substr(0, commaPos);
                    forInfo["index"] = inner.substr(commaPos + 1);
                }
            } else {
                forInfo["item"] = left;
            }
            forInfo["collection"] = collection;
        }
        node["v_for"] = forInfo;

        // Check for <empty> child
        for (auto& child : xmlNode.children()) {
            if (std::string(child.name()) == "empty" || std::string(child.name()) == "else") {
                json emptyChildren = json::array();
                for (auto& ec : child.children()) {
                    emptyChildren.push_back(convertNode(state, ec, parentLoopVars));
                }
                node["v_for_empty"] = emptyChildren;
            } else {
                children.push_back(convertNode(state, child, parentLoopVars));
            }
        }
        node["children"] = children;
        return node;
    }

    // Check for v_if / v_elseif / v_else chain
    std::string vIf = xmlNode.attribute("v_if").value();
    if (!vIf.empty()) {
        node["v_if"] = vIf;
        // Check for subsequent siblings that are v_elseif / v_else
        json elseBranches = json::array();
        pugi::xml_node next = xmlNode;
        while ((next = next.next_sibling())) {
            std::string name = next.name();
            if (name == elemName) {
                pugi::xml_attribute elseIfAttr = next.attribute("v_elseif");
                pugi::xml_attribute elseAttr = next.attribute("v_else");
                if (elseIfAttr || elseAttr) {
                    json branch = convertNode(state, next, parentLoopVars);
                    branch["type"] = name;
                    elseBranches.push_back(branch);
                    continue;
                }
            }
            break;
        }
        if (!elseBranches.empty()) {
            node["v_else_branches"] = elseBranches;
        }
    }

    // Process regular children
    for (auto& child : xmlNode.children()) {
        std::string childName = child.name();
        if (childName == "empty" || childName == "else") continue;
        if (childName.empty() || childName[0] == '\0') continue;
        children.push_back(convertNode(state, child, parentLoopVars));
    }

    if (!children.empty()) {
        node["children"] = children;
    }

    return node;
}

// ─── Compile a layout.ui file ───

static json compileLayout(CompilerState& state, const fs::path& layoutFile) {
    pugi::xml_document doc;
    if (!doc.load_file(layoutFile.string().c_str())) {
        state.error("Failed to parse layout: " + layoutFile.filename().string());
        return nullptr;
    }

    // The layout.ui should be a single widget (usually <window> or <column>)
    auto root = doc.first_child();
    if (!root) {
        state.error("Empty layout file");
        return nullptr;
    }

    json output;
    output["id"] = layoutFile.parent_path().filename().string();

    // If the root has an explicit id attribute, use that
    std::string explicitId = root.attribute("id").value();
    if (!explicitId.empty()) {
        output["id"] = explicitId;
    }

    // Optionally embed resources and scripts as metadata
    if (!state.resources.empty()) {
        output["_resources"] = state.resources;
    }
    if (!state.scripts.empty()) {
        output["_scripts"] = state.scripts;
    }

    output["root"] = convertNode(state, root);

    return output;
}

// ─── Compile Single File ───

static bool compileSingle(CompilerState& state, const fs::path& dir, const fs::path& outDir,
    const fs::path& builtinTemplatesDir = "")
{
    fs::path layoutFile = dir / "layout.ui";
    if (!fs::exists(layoutFile)) {
        state.error("No layout.ui found in " + dir.string());
        return false;
    }

    // Load supplementary files
    loadTemplates(state, dir / "templates");
    loadTemplates(state, dir.parent_path() / "templates");
    if (!builtinTemplatesDir.empty()) {
        loadTemplates(state, builtinTemplatesDir);
    }
    loadStyles(state, dir / "styles.xml");
    loadResources(state, dir / "resources.xml");
    loadScripts(state, dir / "scripts.xml");

    // Compile
    json result = compileLayout(state, layoutFile);
    if (result.is_null()) return false;

    // Write output
    fs::path outputPath = outDir / (result["id"].get<std::string>() + ".json");
    std::ofstream out(outputPath.string());
    if (!out.is_open()) {
        state.error("Failed to write output: " + outputPath.string());
        return false;
    }
    out << result.dump(2);
    out.close();

    std::cout << "  ✓ " << dir.filename().string() << " → " << outputPath.filename().string() << std::endl;
    return true;
}

// ─── Print Usage ───

static void printUsage(const char* prog) {
    std::cerr << "FastUI Compiler v1.0" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: " << prog << " [options] <input>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -o, --output <dir>   Output directory (default: ./build)" << std::endl;
    std::cerr << "  -t, --templates <dir> Template directory (default: auto-detect)" << std::endl;
    std::cerr << "  -r, --recursive      Process subdirectories" << std::endl;
    std::cerr << "  -w, --watch          Watch mode (auto-recompile on change)" << std::endl;
    std::cerr << "  -h, --help           Show this help" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Input can be:" << std::endl;
    std::cerr << "  a directory       Compile layout.ui in this directory" << std::endl;
    std::cerr << "  a .ui file        Compile a single layout file" << std::endl;
    std::cerr << "  a glob pattern    Compile multiple layouts" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << prog << " gui_defs/my_panel/ -o dist" << std::endl;
    std::cerr << "  " << prog << " my_layout.ui -o dist" << std::endl;
}

// ─── Main ───

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::vector<std::string> inputs;
    fs::path outDir = fs::current_path() / "build";
    fs::path builtinTemplatesDir;
    bool recursive = false;
    bool watchMode = false;

    // Parse args
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) outDir = argv[++i];
            else { std::cerr << "Error: --output requires a path" << std::endl; return 1; }
        } else if (arg == "-t" || arg == "--templates") {
            if (i + 1 < argc) builtinTemplatesDir = argv[++i];
            else { std::cerr << "Error: --templates requires a path" << std::endl; return 1; }
        } else if (arg == "-r" || arg == "--recursive") {
            recursive = true;
        } else if (arg == "-w" || arg == "--watch") {
            watchMode = true;
        } else {
            inputs.push_back(arg);
        }
    }

    // Detect built-in templates dir (relative to exe)
    if (builtinTemplatesDir.empty()) {
        fs::path exeDir = fs::canonical(fs::path(argv[0])).parent_path();
        fs::path candidate = exeDir.parent_path() / "templates";
        if (fs::exists(candidate)) {
            builtinTemplatesDir = candidate;
        }
    }

    // Set default input if none provided
    if (inputs.empty()) {
        std::cerr << "Error: No input specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    fs::create_directories(outDir);

    int totalCompiled = 0;
    int totalErrors = 0;

    for (auto& input : inputs) {
        fs::path inputPath(input);

        std::vector<fs::path> compileDirs;

        if (fs::is_directory(inputPath)) {
            if (recursive) {
                for (auto& entry : fs::recursive_directory_iterator(inputPath)) {
                    if (entry.is_directory() && fs::exists(entry.path() / "layout.ui")) {
                        compileDirs.push_back(entry.path());
                    }
                }
            } else if (fs::exists(inputPath / "layout.ui")) {
                compileDirs.push_back(inputPath);
            }
        } else if (inputPath.extension() == ".ui") {
            compileDirs.push_back(inputPath.parent_path());
        } else {
            // Try as glob: match all .ui files
            auto parentDir = inputPath.parent_path();
            if (parentDir.empty()) parentDir = ".";
            std::string pattern = inputPath.filename().string();
            if (pattern.find('*') != std::string::npos || pattern.find('?') != std::string::npos) {
                for (auto& entry : fs::directory_iterator(parentDir)) {
                    if (entry.path().extension() == ".ui" && fs::exists(entry.path())) {
                        compileDirs.push_back(entry.path().parent_path());
                    }
                }
            }
        }

        // Watch mode loop
        do {
            if (watchMode && totalCompiled > 0) {
                std::cout << "\n--- Watching for changes (Ctrl+C to exit) ---" << std::endl;
            }

            for (auto& dir : compileDirs) {
                CompilerState state;
                bool ok = compileSingle(state, dir, outDir, builtinTemplatesDir);

                // Print errors/warnings
                for (auto& e : state.errors) {
                    std::cerr << "  ✗ Error: " << e << std::endl;
                    totalErrors++;
                }
                for (auto& w : state.warnings) {
                    std::cerr << "  ⚠ Warning: " << w << std::endl;
                }

                if (ok) totalCompiled++;
            }

            if (watchMode) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } while (watchMode);
    }

    if (totalErrors > 0) {
        std::cerr << "\nCompiled " << totalCompiled << " files with " << totalErrors << " errors" << std::endl;
        return 1;
    }

    if (totalCompiled > 0 && !watchMode) {
        std::cout << "\nDone. " << totalCompiled << " file(s) compiled to " << outDir.string() << std::endl;
    }

    return 0;
}
