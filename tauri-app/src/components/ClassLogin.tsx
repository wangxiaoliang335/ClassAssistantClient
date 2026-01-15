import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getCurrentWindow, LogicalSize } from '@tauri-apps/api/window';
import { X, Minus, Sparkles, GraduationCap, MonitorPlay } from 'lucide-react';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

const ClassLogin = ({ onLoginSuccess }: { onLoginSuccess: (data: any) => void }) => {
    const [classCode, setClassCode] = useState('');
    const [status, setStatus] = useState('');

    // Resize window on mount to ensure it fits the new design
    useEffect(() => {
        const initWindow = async () => {
            try {
                // Set to 800x500 as requested for a landscape layout
                await getCurrentWindow().setSize(new LogicalSize(800, 500));
                await getCurrentWindow().center();
            } catch (e) {
                console.error("Failed to resize window:", e);
            }
        };
        initWindow();
    }, []);

    const handleMinimize = (e: React.MouseEvent) => {
        e.stopPropagation();
        getCurrentWindow().minimize();
    };

    const handleClose = (e: React.MouseEvent) => {
        e.stopPropagation();
        getCurrentWindow().close();
    };

    const handleLogin = async () => {
        if (!classCode.trim()) {
            setStatus('请输入班级编号');
            return;
        }

        try {
            setStatus('正在登录...');
            const response = await invoke('class_login', { classNumber: classCode }) as string;

            let json;
            try {
                json = JSON.parse(response);
            } catch (e) {
                setStatus('服务器响应错误');
                return;
            }

            if (json.data && json.data.code === 200) {
                setStatus('登录成功');
                console.log("Class Login Response:", json.data);
                localStorage.setItem('user_info', JSON.stringify({ ...json.data, loginType: 'class' }));

                setTimeout(() => onLoginSuccess({ ...json.data, loginType: 'class' }), 500);
            } else {
                setStatus(json.data?.message || '登录失败');
            }
        } catch (error) {
            setStatus('请求失败: ' + String(error));
        }
    };

    return (
        <div className="flex h-screen w-screen bg-slate-50 rounded-xl shadow-2xl overflow-hidden font-sans select-none border border-black/5 relative">

            {/* Header / Draggable Area (Absolute over both sides) */}
            <div
                data-tauri-drag-region
                className="h-12 w-full flex items-center justify-between px-4 absolute top-0 left-0 z-50"
                onMouseDown={() => getCurrentWindow().startDragging()}
            >
                {/* Logo visible only on white side if text is dark, or always visible? 
                    Let's make it smart. Left side is blue, Right side is white. 
                */}
                <div className="flex items-center gap-2 opacity-90 pointer-events-none text-white ml-2">
                    {/* Icon mostly for left side visibility */}
                </div>

                <div className="flex items-center gap-2">
                    <button
                        onClick={handleMinimize}
                        onMouseDown={(e) => e.stopPropagation()}
                        className="p-1.5 hover:bg-black/5 rounded-full text-gray-400 hover:text-gray-600 transition-all cursor-pointer"
                    >
                        <Minus size={18} />
                    </button>
                    <button
                        onClick={handleClose}
                        onMouseDown={(e) => e.stopPropagation()}
                        className="p-1.5 hover:bg-rose-500 hover:shadow-md hover:text-white rounded-full text-gray-400 transition-all cursor-pointer"
                    >
                        <X size={18} />
                    </button>
                </div>
            </div>

            {/* Left Side - Illustration / Branding (40%) */}
            <div className="w-[45%] h-full bg-gradient-to-br from-blue-600 to-cyan-500 relative flex flex-col justify-center items-center p-8 text-white overflow-hidden">
                {/* Background Shapes */}
                <div className="absolute top-[-20%] left-[-20%] w-60 h-60 bg-white/10 rounded-full blur-3xl"></div>
                <div className="absolute bottom-[-10%] right-[-10%] w-40 h-40 bg-indigo-500/30 rounded-full blur-2xl"></div>

                {/* Content */}
                <div className="relative z-10 flex flex-col items-center text-center">
                    <div className="w-20 h-20 bg-white/20 backdrop-blur-md rounded-3xl flex items-center justify-center shadow-lg mb-8 ring-1 ring-white/30">
                        <GraduationCap size={40} className="text-white drop-shadow-sm" />
                    </div>

                    <h1 className="text-3xl font-bold mb-3 tracking-tight drop-shadow-md">智慧课堂</h1>
                    <p className="text-blue-100/90 text-sm font-medium tracking-wide">
                        以科技赋能教育<br />
                        让每一个知识点都熠熠生辉
                    </p>
                </div>

                <div className="absolute bottom-6 left-6 text-xs text-blue-200/60 font-mono">
                    Ver 1.0.0 Student Client
                </div>
            </div>

            {/* Right Side - Login Form (60%) */}
            <div className="flex-1 bg-white flex flex-col justify-center items-center p-12 relative">

                <div className="w-full max-w-sm space-y-8">

                    <div className="text-center">
                        <div className="w-12 h-12 bg-blue-50 rounded-2xl flex items-center justify-center mx-auto mb-4 text-blue-600">
                            <MonitorPlay size={24} />
                        </div>
                        <h2 className="text-2xl font-extrabold text-gray-800">班级登录</h2>
                        <p className="text-gray-400 text-sm mt-1">请输入班级编号激活终端</p>
                    </div>

                    <div className="space-y-5">
                        <div className="relative">
                            <label className="text-xs font-semibold text-gray-500 mb-1.5 block uppercase tracking-wider ml-1">班级编号</label>
                            <input
                                type="text"
                                placeholder="例如: 1001"
                                className="w-full bg-gray-50 text-gray-800 placeholder-gray-400 border border-gray-200 focus:bg-white focus:border-blue-500 focus:ring-4 focus:ring-blue-500/10 rounded-xl py-3.5 px-4 outline-none transition-all duration-200 text-lg font-medium tracking-wide"
                                value={classCode}
                                onChange={(e) => setClassCode(e.target.value)}
                                onKeyDown={(e) => e.key === 'Enter' && handleLogin()}
                                autoFocus
                            />
                        </div>

                        <button
                            onClick={handleLogin}
                            className="w-full bg-[#2563eb] hover:bg-[#1d4ed8] active:bg-[#1e40af] text-white font-bold py-4 rounded-xl transition-all duration-200 shadow-lg shadow-blue-500/30 hover:shadow-blue-500/40 transform hover:-translate-y-0.5 active:translate-y-0 flex items-center justify-center gap-2 group"
                        >
                            <Sparkles size={18} className="text-blue-200 group-hover:text-white transition-colors" />
                            <span>立即进入课堂</span>
                        </button>
                    </div>

                    {/* Status Message */}
                    <div className="h-6 flex items-center justify-center w-full">
                        <span className={cn(
                            "text-sm font-medium px-4 py-1.5 rounded-full transition-all duration-300 flex items-center gap-2",
                            status.includes('成功') ? "text-emerald-700 bg-emerald-50" : "text-rose-600 bg-rose-50",
                            status ? "opacity-100 scale-100" : "opacity-0 scale-95"
                        )}>
                            {status && (
                                <>
                                    <span className={cn("w-2 h-2 rounded-full", status.includes('成功') ? "bg-emerald-500" : "bg-rose-500")}></span>
                                    {status}
                                </>
                            )}
                        </span>
                    </div>

                </div>
            </div>

        </div>
    );
};

export default ClassLogin;
