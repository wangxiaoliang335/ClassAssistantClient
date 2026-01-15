import { X, Check, Bell, User, Users, Clock } from "lucide-react";
import { useEffect, useState } from "react";

export interface NotificationItem {
    id: number;
    sender_id: number;
    sender_name: string;
    receiver_id: string;
    unique_group_id: string; // Group ID
    group_name: string;
    content: string;
    content_text: number; // 3 seems to be Invite?
    is_read: number;
    is_agreed: number; // 0=Pending, 1=Agreed, 2=Rejected
    remark?: string;
    created_at: string;
    updated_at: string;
}

interface Props {
    isOpen: boolean;
    onClose: () => void;
    notifications: NotificationItem[];
}

const NotificationCenterModal = ({ isOpen, onClose, notifications }: Props) => {
    if (!isOpen) return null;

    // Filter to show pending items at top
    const sorted = [...notifications].sort((a, b) => {
        // Sort by date desc
        return new Date(b.created_at).getTime() - new Date(a.created_at).getTime();
    });

    return (
        <div className="fixed inset-0 z-[100] flex items-start justify-end p-4 pt-16 bg-transparent pointer-events-none">
            {/* Modal Container - Pointer events auto to capture clicks */}
            <div className="bg-white pointer-events-auto rounded-2xl shadow-2xl w-[380px] max-h-[600px] flex flex-col border border-gray-100 overflow-hidden animate-in slide-in-from-right-5 duration-300">
                {/* Header */}
                <div className="p-4 border-b border-gray-100 bg-gray-50/50 flex items-center justify-between">
                    <div className="flex items-center gap-2">
                        <div className="w-8 h-8 bg-red-50 text-red-500 rounded-lg flex items-center justify-center">
                            <Bell size={16} />
                        </div>
                        <div>
                            <h3 className="font-bold text-gray-800">消息中心</h3>
                            <span className="text-xs text-gray-400">{sorted.length} 条通知</span>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="p-1.5 hover:bg-gray-200 rounded-lg text-gray-400 hover:text-gray-600 transition-colors"
                    >
                        <X size={18} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-y-auto p-2 bg-gray-50/30">
                    {sorted.length === 0 ? (
                        <div className="h-64 flex flex-col items-center justify-center text-gray-400 gap-2">
                            <Bell size={32} className="opacity-20" />
                            <span className="text-sm">暂无新消息</span>
                        </div>
                    ) : (
                        <div className="space-y-2">
                            {sorted.map(item => (
                                <div key={item.id} className="bg-white p-3 rounded-xl border border-gray-100 shadow-sm hover:shadow-md transition-all">
                                    <div className="flex items-start justify-between gap-3">
                                        {/* Icon */}
                                        <div className="mt-0.5">
                                            {item.content_text === 3 ? ( // 3 = Group Invite
                                                <div className="w-9 h-9 bg-blue-50 text-blue-500 rounded-full flex items-center justify-center border border-blue-100">
                                                    <Users size={16} />
                                                </div>
                                            ) : (
                                                <div className="w-9 h-9 bg-purple-50 text-purple-500 rounded-full flex items-center justify-center border border-purple-100">
                                                    <User size={16} />
                                                </div>
                                            )}
                                        </div>

                                        {/* Content */}
                                        <div className="flex-1 min-w-0">
                                            <div className="flex items-center justify-between mb-0.5">
                                                <span className="font-bold text-gray-700 text-sm truncate pr-2">
                                                    {/* Display Group Name if available, else Sender */}
                                                    {item.group_name || item.sender_name}
                                                </span>
                                                <span className="text-[10px] text-gray-400 flex-shrink-0 flex items-center gap-1">
                                                    <Clock size={10} />
                                                    {/* Format date: Remove seconds? Or just show time if today? For now split space */}
                                                    {item.created_at.split(' ')[0]}
                                                </span>
                                            </div>

                                            <p className="text-xs text-gray-500 mb-2 leading-relaxed">
                                                <span className="font-medium text-gray-900">{item.sender_name}</span> {item.content}
                                            </p>

                                            {/* Action Buttons Removed - Default Agreed */}
                                        </div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default NotificationCenterModal;
