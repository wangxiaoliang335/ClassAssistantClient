import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

interface ResetPasswordProps {
    onBack: () => void;
    onSuccess: () => void;
}

const ResetPassword = ({ onBack, onSuccess }: ResetPasswordProps) => {
    const [phone, setPhone] = useState('');
    const [code, setCode] = useState('');
    const [newPassword, setNewPassword] = useState('');
    const [confirmPwd, setConfirmPwd] = useState('');
    const [status, setStatus] = useState('');
    const [countdown, setCountdown] = useState(0);

    // Countdown logic
    useEffect(() => {
        if (countdown > 0) {
            const timer = setTimeout(() => setCountdown(countdown - 1), 1000);
            return () => clearTimeout(timer);
        }
    }, [countdown]);

    const handleGetCode = async () => {
        if (!/^1[3-9]\d{9}$/.test(phone)) {
            setStatus('请输入有效的11位手机号码');
            return;
        }

        try {
            await invoke('send_verification_code', { phone });
            setCountdown(60);
            setStatus('验证码已发送');
        } catch (e) {
            setStatus('获取验证码失败: ' + String(e));
        }
    };

    const handleReset = async () => {
        if (!phone) return setStatus('请输入手机号');
        if (!code) return setStatus('请输入验证码');
        if (!newPassword) return setStatus('请输入新密码');
        if (newPassword !== confirmPwd) return setStatus('两次输入密码不一致');

        try {
            setStatus('正在重置...');
            const response = await invoke('reset_password', {
                phone,
                newPassword: newPassword,
                verificationCode: code
            }) as string;

            let json;
            try {
                json = JSON.parse(response);
            } catch (e) {
                setStatus('服务器响应错误');
                return;
            }

            if (json.data && json.data.code === 200) {
                setStatus('重置成功');
                setTimeout(() => {
                    onSuccess();
                }, 500);
            } else {
                setStatus(json.data?.message || '重置失败');
            }
        } catch (error) {
            setStatus('请求失败: ' + String(error));
        }
    };

    return (
        <div className="flex flex-col gap-3">
            {/* Input Group */}
            <div className="flex flex-col gap-3">
                <div className="group relative">
                    <input
                        type="text"
                        placeholder="请输入手机号"
                        className="peer w-full py-2 px-4 bg-slate-50 border border-slate-200 rounded-xl outline-none text-slate-700 text-sm placeholder:text-slate-400 transition-all focus:bg-white focus:border-blue-400 focus:ring-4 focus:ring-blue-500/10"
                        value={phone}
                        onChange={(e) => setPhone(e.target.value)}
                    />
                </div>

                <div className="group relative flex items-center">
                    <input
                        type="text"
                        placeholder="请输入验证码"
                        className="peer w-full py-2 px-4 bg-slate-50 border border-slate-200 rounded-xl outline-none text-slate-700 text-sm placeholder:text-slate-400 transition-all focus:bg-white focus:border-blue-400 focus:ring-4 focus:ring-blue-500/10"
                        value={code}
                        onChange={(e) => setCode(e.target.value)}
                    />
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
                </div>

                <div className="group relative">
                    <input
                        type="password"
                        placeholder="请输入新密码"
                        className="peer w-full py-2 px-4 bg-slate-50 border border-slate-200 rounded-xl outline-none text-slate-700 text-sm placeholder:text-slate-400 transition-all focus:bg-white focus:border-blue-400 focus:ring-4 focus:ring-blue-500/10"
                        value={newPassword}
                        onChange={(e) => setNewPassword(e.target.value)}
                    />
                </div>

                <div className="group relative">
                    <input
                        type="password"
                        placeholder="请再次输入新密码"
                        className="peer w-full py-2 px-4 bg-slate-50 border border-slate-200 rounded-xl outline-none text-slate-700 text-sm placeholder:text-slate-400 transition-all focus:bg-white focus:border-blue-400 focus:ring-4 focus:ring-blue-500/10"
                        value={confirmPwd}
                        onChange={(e) => setConfirmPwd(e.target.value)}
                    />
                </div>
            </div>

            {/* Action Row */}
            <button
                onClick={handleReset}
                className="w-full mt-2 bg-gradient-to-r from-blue-500 to-blue-600 hover:from-blue-600 hover:to-blue-700 text-white py-2.5 rounded-xl shadow-lg shadow-blue-500/20 text-sm font-semibold tracking-wide transition-all active:scale-[0.98] hover:shadow-blue-500/30"
            >
                确认重置
            </button>

            {/* Status Message */}
            <div className={cn(
                "text-center text-xs font-medium transition-all duration-300 h-4 mt-1 flex items-center justify-center",
                status ? "opacity-100" : "opacity-0"
            )}>
                <span className={status.includes('成功') ? "text-emerald-500 bg-emerald-50 px-2 rounded" : "text-rose-500 bg-rose-50 px-2 rounded"}>
                    {status}
                </span>
            </div>

            {/* Back Link */}
            <div className="text-center pb-2">
                <button onClick={onBack} className="text-xs text-slate-400 hover:text-blue-600 hover:underline transition-colors">
                    想起密码？直接登录
                </button>
            </div>
        </div>
    );
};

export default ResetPassword;
