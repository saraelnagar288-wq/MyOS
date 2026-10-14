import { useEffect, useMemo, useState } from 'react';
import {
  Activity, Battery, Calculator as CalculatorIcon, Check, ChevronDown, ChevronLeft,
  ChevronRight, Clock3, Code2, File, FileText, Folder, FolderOpen, HardDrive,
  Home, Minus, Monitor, MoreHorizontal, Search, Settings as SettingsIcon,
  ShieldCheck, Terminal as TerminalIcon, X, Maximize2, Wifi, Power, Cpu, MemoryStick,
  AppWindow, Trash2, Save, Play, RotateCcw, Plus, Equal
} from 'lucide-react';

type AppId = 'files' | 'editor' | 'settings' | 'terminal' | 'calculator';
type WindowInfo = { id: AppId; title: string; icon: typeof Folder; x: number; y: number; w: number; h: number };

const initialWindows: WindowInfo[] = [
  { id: 'files', title: 'File Explorer', icon: Folder, x: 1.2, y: 1.5, w: 35, h: 43 },
  { id: 'editor', title: 'Text Editor', icon: FileText, x: 36.8, y: 1.5, w: 29, h: 43 },
  { id: 'settings', title: 'Settings', icon: SettingsIcon, x: 67.3, y: 1.5, w: 27, h: 43 },
  { id: 'terminal', title: 'Terminal', icon: TerminalIcon, x: 1.2, y: 47, w: 28, h: 39 },
  { id: 'calculator', title: 'Calculator', icon: CalculatorIcon, x: 30.2, y: 47, w: 15, h: 39 },
];

const files = [
  { name: 'bin', type: 'Directory', size: '—' },
  { name: 'boot', type: 'Directory', size: '—' },
  { name: 'dev', type: 'Directory', size: '—' },
  { name: 'etc', type: 'Directory', size: '—' },
  { name: 'home', type: 'Directory', size: '—' },
  { name: 'lib', type: 'Directory', size: '—' },
  { name: 'mnt', type: 'Directory', size: '—' },
  { name: 'proc', type: 'Directory', size: '—' },
  { name: 'tmp', type: 'Directory', size: '—' },
  { name: 'usr', type: 'Directory', size: '—' },
  { name: 'var', type: 'Directory', size: '—' },
  { name: 'README.txt', type: 'Text file', size: '2 KB' },
];

function WindowBar({ title, icon: Icon, onClose, onMinimize }: { title: string; icon: typeof Folder; onClose: () => void; onMinimize: () => void }) {
  return <div className="window-bar"><div className="window-title"><Icon size={13}/><span>{title}</span></div><div className="window-controls"><button aria-label="Minimize" onClick={onMinimize}><Minus size={12}/></button><button aria-label="Maximize" onClick={() => {}}><Maximize2 size={11}/></button><button aria-label="Close" onClick={onClose}><X size={13}/></button></div></div>;
}

export default function App() {
  const [open, setOpen] = useState<AppId[]>(['files','editor','settings','terminal','calculator']);
  const [minimized, setMinimized] = useState<AppId[]>([]);
  const [active, setActive] = useState<AppId>('terminal');
  const [clock, setClock] = useState(new Date());
  const [path, setPath] = useState('/');
  const [selectedFile, setSelectedFile] = useState('README.txt');
  const [text, setText] = useState('# Welcome to MyOS\n\nMyOS Desktop Environment\n\nA small, independent operating system project.\n');
  const [command, setCommand] = useState('');
  const [terminalLines, setTerminalLines] = useState<string[]>(['MyOS Terminal [Version 0.4.2]', 'Type "help" to see available commands.', '']);
  const [expression, setExpression] = useState('');
  const [calcResult, setCalcResult] = useState('0');
  const [search, setSearch] = useState('');
  const [dark, setDark] = useState(true);
  const [accent, setAccent] = useState('teal');
  const [showLauncher, setShowLauncher] = useState(false);

  useEffect(() => {
    const timer = window.setInterval(() => setClock(new Date()), 1000);
    return () => window.clearInterval(timer);
  }, []);

  const visibleFiles = useMemo(() => files.filter(f => f.name.toLowerCase().includes(search.toLowerCase())), [search]);

  const launch = (id: AppId) => {
    setOpen(prev => prev.includes(id) ? prev : [...prev, id]);
    setMinimized(prev => prev.filter(x => x !== id));
    setActive(id);
    setShowLauncher(false);
  };
  const close = (id: AppId) => {
    setOpen(prev => prev.filter(x => x !== id));
    setMinimized(prev => prev.filter(x => x !== id));
  };
  const minimize = (id: AppId) => {
    setMinimized(prev => prev.includes(id) ? prev : [...prev, id]);
    const next = open.filter(x => x !== id && !minimized.includes(x));
    if (next.length) setActive(next[next.length - 1]);
  };
  const runCommand = () => {
    const cmd = command.trim();
    if (!cmd) return;
    let output: string[] = [];
    if (cmd === 'help') output = ['help  ls  pwd  whoami  uname  clear  date  neofetch'];
    else if (cmd === 'ls') output = [files.map(f => f.name).join('   ')];
    else if (cmd === 'pwd') output = [path];
    else if (cmd === 'whoami') output = ['user'];
    else if (cmd === 'uname') output = ['MyOS 0.4.2 x86_64'];
    else if (cmd === 'date') output = [new Date().toString()];
    else if (cmd === 'clear') { setTerminalLines([]); setCommand(''); return; }
    else if (cmd === 'neofetch') output = ['  MYOS  |  OS: MyOS 0.4.2', '        |  Kernel: MyOS kernel', '  x64   |  Architecture: x86_64', '        |  Desktop: MyOS DE', '        |  Status: running'];
    else output = [cmd.split(' ')[0] + ': command not found'];
    setTerminalLines(prev => [...prev, 'user@myos:~$ ' + cmd, ...output]);
    setCommand('');
  };
  const calculate = (key: string) => {
    if (key === 'C') { setExpression(''); setCalcResult('0'); return; }
    if (key === '⌫') { setExpression(prev => prev.slice(0, -1)); return; }
    if (key === '=') {
      try {
        const safe = expression.replace(/×/g, '*').replace(/÷/g, '/');
        if (!/^[0-9+\-*/().% ]+$/.test(safe)) throw new Error('invalid');
        const value = Function('"use strict"; return (' + safe + ')')();
        setCalcResult(Number.isFinite(value) ? String(Number(value.toFixed(8))) : 'Error');
      } catch { setCalcResult('Error'); }
      return;
    }
    setExpression(prev => prev + key);
  };

  return <main className={`desktop ${dark ? 'dark' : 'light'} accent-${accent}`}>
    <div className="desktop-glow glow-one"/><div className="desktop-glow glow-two"/>
    <div className="desktop-brand"><span className="brand-mark">M</span><span>MyOS</span><span className="brand-divider">/</span><span className="brand-caption">Desktop Environment</span></div>
    <div className="desktop-status"><span><Wifi size={12}/> Connected</span><span><ShieldCheck size={12}/> System ready</span></div>

    {open.includes('files') && !minimized.includes('files') && <section className={`window file-window ${active === 'files' ? 'focused' : ''}`} style={{left:'1.2%',top:'7%',width:'35%',height:'39%'}} onMouseDown={() => setActive('files')}>
      <WindowBar title="File Explorer" icon={FolderOpen} onClose={() => close('files')} onMinimize={() => minimize('files')}/>
      <div className="file-toolbar"><button onClick={() => setPath('/')}><Home size={12}/></button><button onClick={() => setPath(path === '/' ? '/home' : '/')}><ChevronLeft size={13}/></button><div className="path-box"><HardDrive size={11}/><span>{path}</span><ChevronDown size={11}/></div><button><MoreHorizontal size={14}/></button></div>
      <div className="file-columns"><span>Name</span><span>Type</span><span>Size</span></div>
      <div className="file-list">{visibleFiles.map(f => <button className={selectedFile === f.name ? 'file-row selected' : 'file-row'} key={f.name} onClick={() => {setSelectedFile(f.name); if(f.name.endsWith('.txt')) setText('# ' + f.name + '\n\nWelcome to MyOS.\n');}}><span className="file-name">{f.type === 'Directory' ? <Folder size={12} className="folder-icon"/> : <FileText size={12}/>} {f.name}</span><span>{f.type}</span><span>{f.size}</span></button>)}</div>
      <div className="window-status"><span>{visibleFiles.length} items</span><span>MyOS File System</span></div>
    </section>}

    {open.includes('editor') && !minimized.includes('editor') && <section className={`window editor-window ${active === 'editor' ? 'focused' : ''}`} style={{left:'36.8%',top:'7%',width:'29%',height:'39%'}} onMouseDown={() => setActive('editor')}>
      <WindowBar title="Text Editor" icon={FileText} onClose={() => close('editor')} onMinimize={() => minimize('editor')}/>
      <div className="editor-toolbar"><span className="file-tab"><File size={11}/>{selectedFile}</span><span className="saved-indicator"><Check size={10}/> Ready</span><button title="Save"><Save size={12}/></button></div>
      <div className="editor-body"><div className="line-numbers">{text.split('\n').map((_,i)=><span key={i}>{i+1}</span>)}</div><textarea aria-label="Text editor" spellCheck={false} value={text} onChange={e=>setText(e.target.value)}/></div>
      <div className="window-status"><span>Ln {text.split('\n').length}, Col 1</span><span>UTF-8 · Plain Text</span></div>
    </section>}

    {open.includes('settings') && !minimized.includes('settings') && <section className={`window settings-window ${active === 'settings' ? 'focused' : ''}`} style={{left:'67.3%',top:'7%',width:'27%',height:'39%'}} onMouseDown={() => setActive('settings')}>
      <WindowBar title="Settings" icon={SettingsIcon} onClose={() => close('settings')} onMinimize={() => minimize('settings')}/>
      <div className="settings-content"><div className="settings-heading"><div className="settings-logo"><Monitor size={17}/></div><div><strong>System Information</strong><small>MyOS Device</small></div></div>
        <div className="settings-grid"><div><span>OS</span><strong>MyOS 0.4.2</strong></div><div><span>Architecture</span><strong>x86_64 (64-bit)</strong></div><div><span>Kernel</span><strong>MyOS Kernel</strong></div><div><span>Display</span><strong>1024 × 768</strong></div><div><span>Session</span><strong>MyOS Desktop</strong></div><div><span>Uptime</span><strong>00:08:14</strong></div></div>
        <div className="settings-section"><span>Appearance</span><div className="appearance-row"><button className={dark?'choice active':''} onClick={()=>setDark(true)}>Dark</button><button className={!dark?'choice active':''} onClick={()=>setDark(false)}>Light</button><span className="accent-label">Accent</span>{['teal','blue','violet'].map(a=><button key={a} aria-label={a+' accent'} className={'accent-dot '+a+(accent===a?' chosen':'')} onClick={()=>setAccent(a)}/>)}</div></div>
      </div><div className="window-status"><span><Activity size={10}/> All systems operational</span><span>Settings</span></div>
    </section>}

    {open.includes('terminal') && !minimized.includes('terminal') && <section className={`window terminal-window ${active === 'terminal' ? 'focused' : ''}`} style={{left:'1.2%',top:'48%',width:'28%',height:'38%'}} onMouseDown={() => setActive('terminal')}>
      <WindowBar title="Terminal" icon={TerminalIcon} onClose={() => close('terminal')} onMinimize={() => minimize('terminal')}/>
      <div className="terminal-output">{terminalLines.map((line,i)=><div key={i} className={i===0?'terminal-muted':line.startsWith('user@')?'terminal-prompt':''}>{line}</div>)}<div className="terminal-input-line"><span>user@myos:~$</span><input aria-label="Terminal command" value={command} onChange={e=>setCommand(e.target.value)} onKeyDown={e=>e.key==='Enter'&&runCommand()} autoComplete="off"/><button onClick={runCommand} aria-label="Run command"><Play size={10}/></button></div></div>
      <div className="window-status"><span>Shell: mysh</span><span>Exit code: 0</span></div>
    </section>}

    {open.includes('calculator') && !minimized.includes('calculator') && <section className={`window calculator-window ${active === 'calculator' ? 'focused' : ''}`} style={{left:'30.2%',top:'48%',width:'15%',height:'38%'}} onMouseDown={() => setActive('calculator')}>
      <WindowBar title="Calculator" icon={CalculatorIcon} onClose={() => close('calculator')} onMinimize={() => minimize('calculator')}/>
      <div className="calc-display"><small>{expression || ' '}</small><strong>{calcResult}</strong></div><div className="calc-grid">{['C','(',')','÷','7','8','9','×','4','5','6','-','1','2','3','+','⌫','0','.','='].map((k,i)=><button key={k} className={k==='='?'equals':i<4?'calc-utility':['÷','×','-','+'].includes(k)?'calc-op':''} onClick={()=>calculate(k)}>{k}</button>)}</div>
    </section>}

    <div className="desktop-note"><div className="note-icon"><Code2 size={14}/></div><div><strong>Welcome to MyOS</strong><span>Your workspace, your system.</span></div></div>
    <footer className="taskbar"><button className="start-button" onClick={()=>setShowLauncher(v=>!v)} title="Applications"><span className="start-mark">M</span></button><div className="taskbar-divider"/>{(['files','editor','settings','terminal','calculator'] as AppId[]).map(id=>{const data=initialWindows.find(w=>w.id===id)!;const Icon=data.icon;return <button key={id} className={`task-app ${open.includes(id)&&!minimized.includes(id)?'running':''} ${active===id?'active':''}`} onClick={()=>open.includes(id)&&!minimized.includes(id)&&active===id?minimize(id):launch(id)} title={data.title}><Icon size={13}/><span>{data.title}</span></button>})}<div className="taskbar-spacer"/><div className="tray"><span className="tray-status"><Wifi size={12}/><Battery size={13}/></span><span className="clock">{clock.toLocaleTimeString([], {hour:'2-digit',minute:'2-digit',hour12:false})}<small>{clock.toLocaleDateString([], {day:'2-digit',month:'short'})}</small></span><button className="power-button" title="Power"><Power size={13}/></button></div></footer>

    {showLauncher && <div className="launcher"><div className="launcher-heading"><span className="start-mark">M</span><div><strong>MyOS</strong><small>Applications</small></div><button onClick={()=>setShowLauncher(false)}><X size={14}/></button></div><div className="launcher-search"><Search size={13}/><input placeholder="Search applications" onChange={e=>setSearch(e.target.value)}/></div><div className="launcher-apps">{initialWindows.map(w=>{const Icon=w.icon;return <button key={w.id} onClick={()=>launch(w.id)}><span><Icon size={18}/></span>{w.title}</button>})}</div><div className="launcher-footer"><span>MyOS 0.4.2</span><button title="Power"><Power size={14}/></button></div></div>}
  </main>;
}
