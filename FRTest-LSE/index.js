const fs = require('fs');
const path = require('path');

const PLUGIN_DIR = __dirname.replace(/\\/g, '/');
const BDS_DIR = path.resolve(PLUGIN_DIR, '..', '..').replace(/\\/g, '/');
const BRIDGE_DIR = BDS_DIR + '/Mods/FastRenderer/gui_bridge';

if (!fs.existsSync(BRIDGE_DIR)) {
    fs.mkdirSync(BRIDGE_DIR, { recursive: true });
}

function log(msg) {
    const t = new Date().toTimeString().slice(0, 8);
    const line = '[' + t + '] [FRTest-LSE] ' + msg + '\n';
    try {
        fs.appendFileSync(BDS_DIR + '/FRTest_LSE.txt', line);
    } catch (e) { }
    try {
        ll.exports(logger, 'info', '[FRTest-LSE] ' + msg);
    } catch (e) { }
}

log('Plugin dir: ' + PLUGIN_DIR);
log('Bridge dir: ' + BRIDGE_DIR);
log('BDS dir: ' + BDS_DIR);

function add(id, rootDef) {
    return { pluginId: 'FRTest-LSE', guiId: id, root: rootDef };
}

function makeAll() {
    const guis = [];
    const keybinds = [];

    keybinds.push({ pluginId: 'FRTest-LSE', bindId: 'lse_key_f8', name: 'F8 LSE Test', vk: 119 });

    guis.push(add('lse_test', {
        type: 'window', props: { title: 'FRTest LSE (桥接)', width: '400', height: '200' },
        children: [
            { type: 'text', props: { value: '通过文件桥从 LSE JS 注册', color: '#4fc3f7' } },
            { type: 'separator', props: {} },
            { type: 'button', props: { text: '测试按钮', color: '#2196f3' } },
            { type: 'checkbox', props: { label: '启用功能', default: 'true' } }
        ]
    }));

    guis.push(add('lse_dashboard', {
        type: 'window', props: { title: 'FR 仪表盘 (LSE)', width: '620', height: '440' },
        children: [
            { type: 'row', props: { spacing: '8' }, children: [
                { type: 'panel', props: { color: '#2d5a8e88', padding: '6', round: '6' }, children: [
                    { type: 'text', props: { value: '玩家在线', color: 'white' } },
                    { type: 'text', props: { value: '12', color: '#4fc3f7' } }
                ] },
                { type: 'panel', props: { color: '#2e7d3288', padding: '6', round: '6' }, children: [
                    { type: 'text', props: { value: 'TPS', color: 'white' } },
                    { type: 'text', props: { value: '20.0', color: '#81c784' } }
                ] },
                { type: 'panel', props: { color: '#b71c1c88', padding: '6', round: '6' }, children: [
                    { type: 'text', props: { value: '内存', color: 'white' } },
                    { type: 'text', props: { value: '256 MB', color: '#ef9a9a' } }
                ] },
                { type: 'panel', props: { color: '#f57f1788', padding: '6', round: '6' }, children: [
                    { type: 'text', props: { value: '在线时长', color: 'white' } },
                    { type: 'text', props: { value: '02:34:12', color: '#ffcc80' } }
                ] }
            ] },
            { type: 'separator', props: {} },
            { type: 'group', props: { label: '系统负载' }, children: [
                { type: 'row', props: { spacing: '8' }, children: [
                    { type: 'text', props: { value: 'CPU:' } },
                    { type: 'progress', props: { value: '0.35', width: '400', height: '18' } }
                ] },
                { type: 'row', props: { spacing: '8' }, children: [
                    { type: 'text', props: { value: '内存:' } },
                    { type: 'progress', props: { value: '0.62', width: '400', height: '18' } }
                ] },
                { type: 'row', props: { spacing: '8' }, children: [
                    { type: 'text', props: { value: '磁盘:' } },
                    { type: 'progress', props: { value: '0.28', width: '400', height: '18' } }
                ] }
            ] },
            { type: 'separator', props: {} },
            { type: 'row', props: { spacing: '8' }, children: [
                { type: 'button', props: { text: '刷新', color: '#2196f3' } },
                { type: 'button', props: { text: '重置', color: '#ff5722' } },
                { type: 'checkbox', props: { label: '自动刷新', default: 'true' } }
            ] }
        ]
    }));

    guis.push(add('lse_settings', {
        type: 'window', props: { title: 'FR 设置 (LSE)', width: '520', height: '420' },
        children: [
            { type: 'group', props: { label: '▸ 基本设置', collapsible: 'true' }, children: [
                { type: 'row', props: { spacing: '8' }, children: [
                    { type: 'text', props: { value: '服务器名称:' } },
                    { type: 'input', props: { placeholder: '输入服务器名...', default: 'Minecraft Server', width: '300' } }
                ] },
                { type: 'row', props: { spacing: '8' }, children: [
                    { type: 'text', props: { value: '最大玩家:' } },
                    { type: 'slider', props: { min: '5', max: '100', default: '20', label: '' } }
                ] },
                { type: 'row', props: { spacing: '8' }, children: [
                    { type: 'text', props: { value: '游戏模式:' } },
                    { type: 'combo', props: { options: '生存,创造,冒险,观察者', label: '', default: '0' } }
                ] }
            ] },
            { type: 'group', props: { label: '▸ 聊天设置', collapsible: 'true' }, children: [
                { type: 'checkbox', props: { label: '启用聊天过滤', default: 'true' } },
                { type: 'checkbox', props: { label: '显示玩家加入/离开消息', default: 'true' } },
                { type: 'checkbox', props: { label: '启用表情符号', default: 'false' } }
            ] },
            { type: 'group', props: { label: '▸ 通知设置', collapsible: 'true' }, children: [
                { type: 'checkbox', props: { label: '允许服务器推送', default: 'true' } },
                { type: 'checkbox', props: { label: '声音提示', default: 'true' } }
            ] },
            { type: 'separator', props: {} },
            { type: 'row', props: { spacing: '8' }, children: [
                { type: 'button', props: { text: '保存设置', color: '#4caf50' } },
                { type: 'button', props: { text: '恢复默认', color: '#ff9800' } }
            ] }
        ]
    }));

    guis.push(add('lse_players', {
        type: 'window', props: { title: 'FR 玩家 (LSE)', width: '360', height: '300' },
        children: [
            { type: 'text', props: { value: '在线玩家 (5人)' } },
            { type: 'separator', props: {} },
            { type: 'group', props: { label: '' }, children: [
                { type: 'row', props: { spacing: '4' }, children: [
                    { type: 'text', props: { value: '▶ Steve', color: '#81c784' } },
                    { type: 'spacer', props: { width: '60' } },
                    { type: 'text', props: { value: '命: 20/20', color: '#90caf9' } }
                ] },
                { type: 'row', props: { spacing: '4' }, children: [
                    { type: 'text', props: { value: '▶ Alex', color: '#81c784' } },
                    { type: 'spacer', props: { width: '60' } },
                    { type: 'text', props: { value: '命: 18/20', color: '#ffcc80' } }
                ] },
                { type: 'row', props: { spacing: '4' }, children: [
                    { type: 'text', props: { value: '▶ Dream', color: '#81c784' } },
                    { type: 'spacer', props: { width: '60' } },
                    { type: 'text', props: { value: '命: 20/20', color: '#90caf9' } }
                ] },
                { type: 'row', props: { spacing: '4' }, children: [
                    { type: 'text', props: { value: '▶ Notch', color: '#81c784' } },
                    { type: 'spacer', props: { width: '60' } },
                    { type: 'text', props: { value: '命: 6/20', color: '#ef9a9a' } }
                ] },
                { type: 'row', props: { spacing: '4' }, children: [
                    { type: 'text', props: { value: '▶ Herobrine', color: '#81c784' } },
                    { type: 'spacer', props: { width: '60' } },
                    { type: 'text', props: { value: '命: ??', color: '#b39ddb' } }
                ] }
            ] },
            { type: 'separator', props: {} },
            { type: 'row', props: { spacing: '8' }, children: [
                { type: 'button', props: { text: '刷新', color: '#2196f3' } },
                { type: 'combo', props: { options: '全部,生存,创造,冒险', label: '过滤:' } }
            ] }
        ]
    }));

    guis.push(add('lse_console', {
        type: 'window', props: { title: 'FR 控制台 (LSE)', width: '500', height: '300' },
        children: [
            { type: 'row', props: { spacing: '4' }, children: [
                { type: 'combo', props: { options: '全部,信息,警告,错误', label: '级别:', default: '0' } },
                { type: 'input', props: { placeholder: '搜索日志...', width: '260' } },
                { type: 'button', props: { text: '清空', color: '#ff5722' } }
            ] },
            { type: 'separator', props: {} },
            { type: 'panel', props: { color: '#0d0d0d66', padding: '6' }, children: [
                { type: 'text', props: { value: '[FR] LSE bridge GUI loaded', color: '#81c784' } },
                { type: 'text', props: { value: '[FR] 5 panels registered', color: '#4caf50' } },
                { type: 'text', props: { value: '[OK] BridgeService polling OK', color: '#90caf9' } }
            ] }
        ]
    }));

    const guiJson = JSON.stringify(guis, null, 2);
    const kbJson = JSON.stringify(keybinds, null, 2);

    fs.writeFileSync(BRIDGE_DIR + '/gui_pending.json', guiJson);
    log('Wrote gui_pending.json: ' + guis.length + ' GUIs (' + guiJson.length + ' bytes)');

    fs.writeFileSync(BRIDGE_DIR + '/keybind_pending.json', kbJson);
    log('Wrote keybind_pending.json: ' + keybinds.length + ' keybinds');
}

try {
    makeAll();
    log('All bridge files written successfully');
} catch (e) {
    log('ERROR: ' + (e.message || e));
}
