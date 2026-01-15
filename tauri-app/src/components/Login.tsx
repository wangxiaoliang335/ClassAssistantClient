import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { Minus, X, ChevronLeft } from 'lucide-react';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';
import Register from './Register';
import ResetPassword from './ResetPassword';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

type ViewState = 'login' | 'register' | 'reset';

const Login = ({ onLoginSuccess }: { onLoginSuccess: (data: any) => void }) => {
    const [view, setView] = useState<ViewState>('login');
    const [loginMode, setLoginMode] = useState<'password' | 'code'>('password');
    const [phone, setPhone] = useState('');
    const [password, setPassword] = useState('');
    const [status, setStatus] = useState('');
    const [countdown, setCountdown] = useState(0);
    const [autoLogin, setAutoLogin] = useState(false);
    const [rememberPwd, setRememberPwd] = useState(false);

    // Countdown logic
    useEffect(() => {
        if (countdown > 0) {
            const timer = setTimeout(() => setCountdown(countdown - 1), 1000);
            return () => clearTimeout(timer);
        }
    }, [countdown]);

    useEffect(() => {
        const prefsStr = localStorage.getItem('login_prefs');
        if (prefsStr) {
            try {
                const prefs = JSON.parse(prefsStr);
                setPhone(prefs.phone || '');
                if (prefs.rememberPwd) {
                    setPassword(prefs.password || '');
                    setRememberPwd(true);
                }
                if (prefs.autoLogin) {
                    setAutoLogin(true);
                    // Attempt auto login
                    if (prefs.phone && prefs.password && prefs.rememberPwd) {
                        performLogin(prefs.phone, prefs.password);
                    }
                }
            } catch (e) { console.error(e); }
        }
    }, []);

    const handleMinimize = () => {
        getCurrentWindow().minimize();
    };

    const handleClose = () => {
        getCurrentWindow().close();
    };

    const handleGetCode = () => {
        if (!/^1[3-9]\d{9}$/.test(phone)) {
            setStatus('请输入有效的11位手机号码');
            return;
        }
        setCountdown(60);
        setStatus('验证码已发送');
        // TODO: Call backend to send code
    };

    const performLogin = async (currentPhone: string, currentPassword: string) => {
        try {
            setStatus('正在登录...');
            const response = await invoke('login', { phone: currentPhone, password: currentPassword }) as string;

            let json;
            try {
                json = JSON.parse(response);
            } catch (e) {
                setStatus('服务器响应错误');
                return;
            }

            if (json.data && json.data.code === 200) {
                setStatus('登录成功');
                // Save Info for WebSocket
                console.log("Login Response Data:", json.data);
                localStorage.setItem('user_info', JSON.stringify({ ...json.data, phone: currentPhone }));

                // Save Prefs
                // Note: We use the state variables for prefs, but need to be careful if this runs from Effect where state might not be updated yet?
                // Actually if auto-login runs, we want to save what was loaded. 
                // But performLogin arguments are reliable.
                // We'll use the current state values for flags, but if coming from auto-login hook, state might be outdated in closure? 
                // In useEffect, we set state then call performLogin. React batching might mean state isn't ready if we called it immediately.
                // But we used prefs values.
                // Let's passed flags as args or rely on localStorage flow.
                // Simplified: On manual login, we use current state. On auto login, we update localStorage if needed?
                // Let's just update localStorage on manual interaction success. 
                // But for auto-login, we don't need to re-save if nothing changed.
                // However, we can just save based on current state (or passed prefs).
                // Issue: accessing `autoLogin` state inside `performLogin` which is called from `useEffect`.
                // If defined inside component, it closes over initial `autoLogin` (false).
                // So AutoLogin will save `false` if re-saved.
                // We should pass flags to performLogin or use refs.
                // I'll stick to updating `handleLogin` wrapper for manual which saves.
                // `performLogin` just logs in.

                // Inject the phone number used for login
                setTimeout(() => onLoginSuccess({ ...json.data, phone: currentPhone }), 200);
            } else {
                setStatus(json.data?.message || '登录失败');
            }
        } catch (error) {
            setStatus('请求失败: ' + String(error));
        }
    };

    const handleLogin = async () => {
        if (!phone) {
            setStatus('请输入账号');
            return;
        }
        if (!password) {
            setStatus(loginMode === 'code' ? '请输入验证码' : '请输入密码');
            return;
        }

        // Save prefs on manual login attempt (or success?)
        // Usually save on success.
        // But here I'll save on attempt to keep logic simple or do it inside performLogin?
        // Let's do it here.
        if (rememberPwd) {
            localStorage.setItem('login_prefs', JSON.stringify({
                phone,
                password,
                rememberPwd,
                autoLogin
            }));
        } else {
            localStorage.setItem('login_prefs', JSON.stringify({
                phone,
                password: "",
                rememberPwd: false,
                autoLogin: false
            }));
        }

        await performLogin(phone, password);
    };

    const getTitle = () => {
        switch (view) {
            case 'register': return '注册账号';
            case 'reset': return '重置密码';
            default: return '教师助手';
        }
    };

    return (
        <div className="flex flex-col h-screen w-screen bg-white rounded-2xl shadow-2xl overflow-hidden font-[system-ui] select-none border border-white/50 relative">

            {/* Header Area - Fresh Gradient for Education Context */}
            <div
                data-tauri-drag-region
                onMouseDown={() => getCurrentWindow().startDragging()}
                className="h-28 bg-gradient-to-br from-sky-500 via-blue-500 to-blue-600 relative shrink-0 cursor-move"
            >
                {/* Visual Decorative Circles */}
                <div className="absolute top-[-20%] left-[-10%] w-40 h-40 bg-white/10 rounded-full blur-2xl pointer-events-none"></div>
                <div className="absolute bottom-[-20%] right-[-10%] w-40 h-40 bg-cyan-400/20 rounded-full blur-2xl pointer-events-none"></div>

                {/* Logo & Text */}
                <div className="absolute top-4 left-4 flex items-center gap-2.5 z-20">
                    {view !== 'login' ? (
                        <button
                            onClick={() => setView('login')}
                            onMouseDown={(e) => e.stopPropagation()}
                            className="flex items-center gap-1 text-white hover:bg-white/20 rounded-lg px-2 py-1 -ml-2 transition-all cursor-pointer active:scale-95"
                        >
                            <ChevronLeft size={20} />
                            <span className="text-white text-sm font-medium tracking-wide shadow-sm">{getTitle()}</span>
                        </button>
                    ) : (
                        <div className="flex items-center gap-2.5 pointer-events-none">
                            <div className="w-8 h-8 bg-white/20 backdrop-blur-sm rounded-lg flex items-center justify-center shadow-inner ring-1 ring-white/30">
                                <div className="w-3 h-3 bg-white rounded-full shadow-[0_0_10px_rgba(255,255,255,0.8)]"></div>
                            </div>
                            <div className="flex flex-col">
                                <span className="text-white text-sm font-bold tracking-wider shadow-sm leading-tight">教师助手</span>
                                <span className="text-blue-100 text-[10px] font-medium tracking-wide">智慧课堂助手</span>
                            </div>
                        </div>
                    )}
                </div>

                {/* Window Controls - Softer buttons */}
                <div className="absolute top-2 right-2 flex items-center gap-1 z-20" onMouseDown={(e) => e.stopPropagation()}>
                    {view === 'login' && (
                        <button
                            onClick={() => setLoginMode(loginMode === 'password' ? 'code' : 'password')}
                            className="text-white/90 hover:text-white text-xs px-3 py-1.5 rounded-full bg-black/10 hover:bg-black/20 transition-all active:scale-95 mr-2 backdrop-blur-sm border border-white/10"
                        >
                            {loginMode === 'password' ? "验证码登录" : "密码登录"}
                        </button>
                    )}
                    <button onClick={handleMinimize} className="p-1.5 hover:bg-white/20 rounded-full text-white transition-colors">
                        <Minus size={16} />
                    </button>
                    <button onClick={handleClose} className="p-1.5 hover:bg-red-500/80 hover:shadow-lg rounded-full text-white transition-all">
                        <X size={16} />
                    </button>
                </div>
            </div>

            {/* Main Body - Clean & Spacious */}
            <div className="flex-1 w-full bg-white relative flex flex-col items-center">

                {/* Background Pattern */}
                <div className="absolute inset-0 bg-[radial-gradient(#e5e7eb_1px,transparent_1px)] [background-size:16px_16px] opacity-30 pointer-events-none"></div>

                {/* Centered Content Container */}
                {view === 'login' && (
                    <div className="w-72 flex flex-col pt-6 relative z-10">

                        {/* Input Group - Card Style */}
                        <div className="flex flex-col gap-3">
                            <div className="group relative">
                                <input
                                    type="text"
                                    placeholder="手机号 / 账号"
                                    className="peer w-full py-2.5 px-4 bg-slate-50 border border-slate-200 rounded-xl outline-none text-slate-700 text-sm placeholder:text-slate-400 transition-all focus:bg-white focus:border-blue-400 focus:ring-4 focus:ring-blue-500/10"
                                    value={phone}
                                    onChange={(e) => setPhone(e.target.value)}
                                />
                            </div>

                            <div className="group relative flex items-center">
                                <input
                                    type={loginMode === 'password' ? "password" : "text"}
                                    placeholder={loginMode === 'password' ? "请输入密码" : "请输入验证码"}
                                    className="peer w-full py-2.5 px-4 bg-slate-50 border border-slate-200 rounded-xl outline-none text-slate-700 text-sm placeholder:text-slate-400 transition-all focus:bg-white focus:border-blue-400 focus:ring-4 focus:ring-blue-500/10"
                                    value={password}
                                    onChange={(e) => setPassword(e.target.value)}
                                />
                                {loginMode === 'code' && (
                                    <button
                                        onClick={handleGetCode}
                                        disabled={countdown > 0}
                                        className={cn(
                                            "absolute right-2 top-1.5 bottom-1.5 text-xs px-3 rounded-lg bg-blue-50 text-blue-600 font-medium hover:bg-blue-100 transition-all",
                                            countdown > 0 && "bg-gray-100 text-gray-400 cursor-not-allowed"
                                        )}
                                    >
                                        {countdown > 0 ? `${countdown}s` : '获取'}
                                    </button>
                                )}
                            </div>
                        </div>

                        {/* Options Row */}
                        <div className="flex justify-between items-center px-1 mt-4">
                            <div className="flex gap-4">
                                <label className="flex items-center gap-2 cursor-pointer group select-none">
                                    <div className="relative flex items-center">
                                        <input
                                            type="checkbox"
                                            checked={autoLogin}
                                            onChange={(e) => setAutoLogin(e.target.checked)}
                                            className="peer h-4 w-4 rounded border-gray-300 text-blue-600 focus:ring-blue-500/20 cursor-pointer transition-all checked:bg-blue-500 checked:border-blue-500"
                                        />
                                    </div>
                                    <span className="text-xs text-slate-500 group-hover:text-slate-700 transition-colors">自动登录</span>
                                </label>
                                <label className="flex items-center gap-2 cursor-pointer group select-none">
                                    <div className="relative flex items-center">
                                        <input
                                            type="checkbox"
                                            checked={rememberPwd}
                                            onChange={(e) => setRememberPwd(e.target.checked)}
                                            className="peer h-4 w-4 rounded border-gray-300 text-blue-600 focus:ring-blue-500/20 cursor-pointer transition-all checked:bg-blue-500 checked:border-blue-500"
                                        />
                                    </div>
                                    <span className="text-xs text-slate-500 group-hover:text-slate-700 transition-colors">记住密码</span>
                                </label>
                            </div>
                        </div>

                        {/* Action Row */}
                        <button
                            onClick={handleLogin}
                            className="w-full mt-5 bg-gradient-to-r from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white py-2.5 rounded-xl shadow-lg shadow-blue-500/20 text-sm font-semibold tracking-wide transition-all active:scale-[0.98] hover:shadow-blue-500/30"
                        >
                            进入课堂
                        </button>

                        {/* Status Message */}
                        <div className={cn(
                            "text-center text-xs font-medium transition-all duration-300 h-5 mt-3 flex items-center justify-center",
                            status ? "opacity-100" : "opacity-0"
                        )}>
                            <span className={status.includes('成功') ? "text-emerald-500 bg-emerald-50 px-3 py-0.5 rounded-full" : "text-rose-500 bg-rose-50 px-3 py-0.5 rounded-full"}>
                                {status}
                            </span>
                        </div>

                        {/* Footer Links */}
                        <div className="absolute bottom-[-20px] left-0 right-0">
                            {/* Adjusted position since we have more padding/space now. 
                                Actually, checking Login structure, bottom-4 links were overlapping with content if content grew. 
                                Let's place them relative or ensure space.
                            */}
                            <div className="flex justify-center items-center gap-6 text-xs text-slate-400 mt-2">
                                <button onClick={() => setView('reset')} className="hover:text-blue-600 hover:underline transition-all">找回密码</button>
                                <div className="w-[1px] h-3 bg-slate-200"></div>
                                <button onClick={() => setView('register')} className="hover:text-blue-600 hover:underline transition-all">注册账号</button>
                            </div>
                        </div>
                    </div>
                )}

                {view === 'register' && (
                    <div className="relative z-10 w-72 pt-4">
                        <Register
                            onBack={() => setView('login')}
                            onSuccess={(data) => {
                                setView('login');
                                setPhone(data.phone || '');
                                setStatus('注册成功，请登录');
                            }}
                        />
                    </div>
                )}

                {view === 'reset' && (
                    <div className="relative z-10 w-72 pt-4">
                        <ResetPassword
                            onBack={() => setView('login')}
                            onSuccess={() => {
                                setView('login');
                                setStatus('密码重置成功，请登录');
                            }}
                        />
                    </div>
                )}

            </div>
        </div>
    );
};

export default Login;
