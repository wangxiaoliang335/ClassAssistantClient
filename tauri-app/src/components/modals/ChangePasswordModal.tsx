import { useState, useEffect } from "react";
import { X, Lock, Phone as PhoneIcon, Key, CheckCircle, Shield } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";

interface ChangePasswordModalProps {
    isOpen: boolean;
    onClose: () => void;
    defaultPhone?: string;
}

const ChangePasswordModal = ({ isOpen, onClose, defaultPhone }: ChangePasswordModalProps) => {
    const [phone, setPhone] = useState("");
    const [password, setPassword] = useState("");
    const [confirmPassword, setConfirmPassword] = useState("");
    const [code, setCode] = useState("");

    const [isSending, setIsSending] = useState(false);
    const [countdown, setCountdown] = useState(0);
    const [isLoading, setIsLoading] = useState(false);
    const [error, setError] = useState("");
    const [success, setSuccess] = useState("");

    useEffect(() => {
        if (isOpen && defaultPhone) {
            setPhone(defaultPhone);
        }
        if (!isOpen) {
            // Reset state on close
            setPassword("");
            setConfirmPassword("");
            setCode("");
            setError("");
            setSuccess("");
        }
    }, [isOpen, defaultPhone]);

    // Countdown timer
    useEffect(() => {
        let timer: any;
        if (countdown > 0) {
            timer = setInterval(() => {
                setCountdown((prev) => prev - 1);
            }, 1000);
        }
        return () => clearInterval(timer);
    }, [countdown]);

    const handleSendCode = async () => {
        if (!phone) {
            setError("请输入手机号码");
            return;
        }
        setIsSending(true);
        setError("");

        try {
            await invoke('send_verification_code', { phone });
            setCountdown(60);
            setError(""); // Clear any previous error
            // Could show success toast
        } catch (e: any) {
            console.error("Failed to send code", e);
            setError("验证码发送失败: " + (e.message || e));
        } finally {
            setIsSending(false);
        }
    };

    const handleSubmit = async () => {
        if (!phone || !password || !code) {
            setError("请填写所有字段");
            return;
        }
        if (password !== confirmPassword) {
            setError("两次输入的密码不一致");
            return;
        }
        if (password.length < 6) {
            setError("密码长度不能少于6位");
            return;
        }

        setIsLoading(true);
        setError("");

        try {
            await invoke('reset_password', {
                phone,
                newPassword: password,
                verificationCode: code
            });
            setSuccess("密码修改成功！请使用新密码重新登录。");
            setTimeout(() => {
                onClose();
                // Optionally trigger logout?
                // window.location.reload(); 
                // But user might just want to change it. Qt usually doesn't force logout unless session invalidated.
            }, 2000);
        } catch (e: any) {
            console.error("Reset password failed", e);
            setError("密码修改失败: " + (e.message || e));
            setIsLoading(false);
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/40 backdrop-blur-sm p-4 animate-in fade-in duration-200">
            <div className="bg-white rounded-xl shadow-2xl w-full max-w-sm overflow-hidden flex flex-col p-6">

                {/* Header */}
                <div className="flex items-center justify-between mb-6">
                    <h2 className="text-xl font-bold text-gray-800 flex items-center gap-2">
                        <Lock className="text-blue-500" size={24} />
                        修改密码
                    </h2>
                    <button onClick={onClose} className="p-1 hover:bg-gray-100 rounded-full text-gray-500">
                        <X size={20} />
                    </button>
                </div>

                {success ? (
                    <div className="flex flex-col items-center justify-center py-8 space-y-4 animate-in zoom-in-50">
                        <div className="w-16 h-16 bg-green-100 text-green-600 rounded-full flex items-center justify-center">
                            <CheckCircle size={32} />
                        </div>
                        <p className="text-lg font-medium text-gray-800">修改成功</p>
                        <p className="text-sm text-gray-500">窗口即将关闭...</p>
                    </div>
                ) : (
                    <div className="space-y-4">
                        {/* Error Msg */}
                        {error && (
                            <div className="p-3 bg-red-50 text-red-600 text-xs rounded-lg border border-red-100 flex items-start gap-2">
                                <Shield size={14} className="mt-0.5 shrink-0" />
                                <span>{error}</span>
                            </div>
                        )}

                        {/* Phone */}
                        <div className="space-y-1">
                            <label className="text-xs font-medium text-gray-500">手机号码</label>
                            <div className="relative">
                                <PhoneIcon className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" size={16} />
                                <input
                                    type="text"
                                    value={phone}
                                    readOnly={!!defaultPhone} // Read only if phone is provided
                                    onChange={(e) => setPhone(e.target.value)}
                                    className={`w-full pl-9 pr-3 py-2 bg-gray-50 border border-gray-200 rounded-lg text-sm outline-none focus:border-blue-500 focus:bg-white transition-all ${defaultPhone ? 'text-gray-500 cursor-not-allowed' : ''}`}
                                    placeholder="请输入手机号"
                                />
                            </div>
                        </div>

                        {/* Code */}
                        <div className="space-y-1">
                            <label className="text-xs font-medium text-gray-500">验证码</label>
                            <div className="flex gap-2">
                                <div className="relative flex-1">
                                    <Key className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" size={16} />
                                    <input
                                        type="text"
                                        value={code}
                                        onChange={(e) => setCode(e.target.value)}
                                        className="w-full pl-9 pr-3 py-2 bg-gray-50 border border-gray-200 rounded-lg text-sm outline-none focus:border-blue-500 focus:bg-white transition-all"
                                        placeholder="请输入验证码"
                                    />
                                </div>
                                <button
                                    onClick={handleSendCode}
                                    disabled={isSending || countdown > 0 || !phone}
                                    className={`px-3 py-2 rounded-lg text-xs font-medium whitespace-nowrap transition-colors ${isSending || countdown > 0 || !phone
                                        ? "bg-gray-100 text-gray-400 cursor-not-allowed"
                                        : "bg-blue-50 text-blue-600 hover:bg-blue-100"
                                        }`}
                                >
                                    {countdown > 0 ? `${countdown}s 后重试` : (isSending ? "发送中..." : "获取验证码")}
                                </button>
                            </div>
                        </div>

                        {/* New Password */}
                        <div className="space-y-1">
                            <label className="text-xs font-medium text-gray-500">新密码</label>
                            <div className="relative">
                                <Lock className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" size={16} />
                                <input
                                    type="password"
                                    value={password}
                                    onChange={(e) => setPassword(e.target.value)}
                                    className="w-full pl-9 pr-3 py-2 bg-gray-50 border border-gray-200 rounded-lg text-sm outline-none focus:border-blue-500 focus:bg-white transition-all"
                                    placeholder="请输入新密码（至少6位）"
                                />
                            </div>
                        </div>

                        {/* Confirm Password */}
                        <div className="space-y-1">
                            <label className="text-xs font-medium text-gray-500">确认密码</label>
                            <div className="relative">
                                <Lock className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400" size={16} />
                                <input
                                    type="password"
                                    value={confirmPassword}
                                    onChange={(e) => setConfirmPassword(e.target.value)}
                                    className="w-full pl-9 pr-3 py-2 bg-gray-50 border border-gray-200 rounded-lg text-sm outline-none focus:border-blue-500 focus:bg-white transition-all"
                                    placeholder="请再次输入新密码"
                                />
                            </div>
                        </div>

                        <button
                            onClick={handleSubmit}
                            disabled={isLoading}
                            className={`w-full py-2.5 mt-4 rounded-xl text-white font-bold text-sm shadow-md transition-all active:scale-95 ${isLoading
                                ? "bg-gray-400 cursor-not-allowed"
                                : "bg-blue-600 hover:bg-blue-500 hover:shadow-lg"
                                }`}
                        >
                            {isLoading ? "提交中..." : "确认修改"}
                        </button>
                    </div>
                )}
            </div>
        </div>
    );
};

export default ChangePasswordModal;
