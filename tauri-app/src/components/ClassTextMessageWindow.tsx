import { useEffect, useState, useRef } from 'react';
import { X, Maximize2, Minimize2, Bell, Inbox, Trash2, ChevronRight, MessageSquare, Info } from 'lucide-react';
import { useDraggable } from '../hooks/useDraggable';

interface Message {
    id: string;
    content: string;
    sender: string;
    senderName: string;
    timestamp: number;
    type?: 'info' | 'alert' | 'success';
}

interface ClassTextMessageWindowProps {
    classCode: string;
    groupId?: string; // 保留以兼容，但不再使用
    isOpen: boolean;
    onClose: () => void;
    receiveNotification: boolean; // 接收通知开关状态
}

const ClassTextMessageWindow = ({
    classCode,
    isOpen,
    onClose,
    receiveNotification
}: ClassTextMessageWindowProps) => {
    const { style, handleMouseDown } = useDraggable();
    const [mode, setMode] = useState<'minimal' | 'full'>('minimal'); // minimal: 极简模式（默认）, full: 完整模式
    const [messages, setMessages] = useState<Message[]>([]);
    const messagesEndRef = useRef<HTMLDivElement>(null);
    const messagesContainerRef = useRef<HTMLDivElement>(null);
    const [isDragging, setIsDragging] = useState(false);
    const dragStartY = useRef(0);
    const scrollStartY = useRef(0);

    // 监听WebSocket通知消息（只接收通知，不发送消息）
    useEffect(() => {
        if (!isOpen || !receiveNotification) return;

        const handleWSMessage = (event: Event) => {
            const customEvent = event as CustomEvent;
            const msgStr = customEvent.detail;
            if (!msgStr) return;

            try {
                const msg = JSON.parse(msgStr);
                // 检查是否是通知消息且是发给当前班级的
                if (msg.type === 'notification' && msg.class_id === classCode) {
                    console.log('[ClassTextMessageWindow] Received notification:', msg);
                    const notificationMessage: Message = {
                        id: `notification_${Date.now()}_${Math.random()}`,
                        content: msg.content || '',
                        sender: msg.sender_id || '',
                        senderName: msg.sender_name || '系统',
                        timestamp: Date.now(),
                        type: msg.notification_type || 'info'
                    };
                    // 保持最多100条消息
                    setMessages(prev => {
                        const newMessages = [...prev, notificationMessage];
                        return newMessages.slice(-100);
                    });

                    // 如果是极简模式，自动切换到完整模式以显示通知
                    if (mode === 'minimal') {
                        setMode('full');
                    }

                    // 自动滚动到底部
                    setTimeout(() => {
                        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
                    }, 100);
                }
            } catch (e) {
                console.error('[ClassTextMessageWindow] Failed to parse WS message:', e);
            }
        };

        window.addEventListener('ws-message', handleWSMessage);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage);
        };
    }, [isOpen, receiveNotification, classCode, mode]);

    // 切换模式
    const toggleMode = () => {
        setMode(prev => prev === 'minimal' ? 'full' : 'minimal');
    };

    const clearMessages = () => {
        if (confirm('确定要清空所有通知吗？')) {
            setMessages([]);
        }
    };

    // 完整模式下的拖动滚动
    const handleMouseDownOnMessages = (e: React.MouseEvent) => {
        if (mode !== 'full') return;
        setIsDragging(true);
        dragStartY.current = e.clientY;
        if (messagesContainerRef.current) {
            scrollStartY.current = messagesContainerRef.current.scrollTop;
        }
        e.preventDefault();
    };

    const handleMouseMove = (e: MouseEvent) => {
        if (!isDragging || mode !== 'full' || !messagesContainerRef.current) return;
        const deltaY = e.clientY - dragStartY.current;
        messagesContainerRef.current.scrollTop = scrollStartY.current - deltaY;
    };

    const handleMouseUp = () => {
        setIsDragging(false);
    };

    useEffect(() => {
        if (isDragging) {
            window.addEventListener('mousemove', handleMouseMove);
            window.addEventListener('mouseup', handleMouseUp);
            return () => {
                window.removeEventListener('mousemove', handleMouseMove);
                window.removeEventListener('mouseup', handleMouseUp);
            };
        }
    }, [isDragging]);

    // 自动滚动到底部
    useEffect(() => {
        if (mode === 'full') {
            setTimeout(() => {
                messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
            }, 100);
        }
    }, [messages, mode]);

    if (!isOpen) return null;

    const latestMessage = messages.length > 0 ? messages[messages.length - 1] : null;

    const formatTime = (ts: number) => {
        const date = new Date(ts);
        const now = new Date();
        if (date.toDateString() === now.toDateString()) {
            return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        }
        return `${date.getMonth() + 1}/${date.getDate()} ${date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}`;
    };

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center pointer-events-none">
            <div
                className={`bg-white/90 backdrop-blur-3xl rounded-3xl shadow-[0_20px_50px_rgba(0,0,0,0.15),0_0_0_1px_rgba(255,255,255,0.5)] flex flex-col pointer-events-auto overflow-hidden border border-white/40 ${
                    mode === 'minimal' ? 'w-[400px] h-[80px]' : 'w-[420px] h-[650px]'
                } transition-all duration-500 ease-in-out`}
                style={style}
            >
                {/* Header */}
                <div
                    className="flex items-center justify-between px-5 py-4 bg-gradient-to-b from-white/80 to-transparent cursor-move shrink-0"
                    onMouseDown={handleMouseDown}
                >
                    <div className="flex items-center gap-3">
                        <div className="relative">
                            <div className="w-10 h-10 rounded-2xl bg-blue-500 flex items-center justify-center shadow-lg shadow-blue-500/20">
                                <Bell className="text-white" size={20} />
                            </div>
                            {messages.length > 0 && (
                                <span className="absolute -top-1 -right-1 w-4 h-4 bg-red-500 border-2 border-white rounded-full"></span>
                            )}
                        </div>
                        <div>
                            <h3 className="text-gray-900 text-sm font-bold tracking-wide">班级通知中心</h3>
                            <p className="text-[10px] text-gray-500 uppercase tracking-widest font-medium">Class Notifications</p>
                        </div>
                    </div>
                    <div className="flex items-center gap-1.5">
                        {mode === 'full' && messages.length > 0 && (
                            <button
                                onClick={clearMessages}
                                className="p-2 hover:bg-red-50 text-gray-400 hover:text-red-500 rounded-xl transition-all duration-200"
                                title="清空所有通知"
                            >
                                <Trash2 size={16} />
                            </button>
                        )}
                        <button
                            onClick={toggleMode}
                            className="p-2 hover:bg-blue-50 text-gray-400 hover:text-blue-500 rounded-xl transition-all duration-200"
                            title={mode === 'minimal' ? '展开通知' : '收起通知'}
                        >
                            {mode === 'minimal' ? <Maximize2 size={16} /> : <Minimize2 size={16} />}
                        </button>
                        <button
                            onClick={onClose}
                            className="p-2 hover:bg-red-50 text-gray-400 hover:text-red-500 rounded-xl transition-all duration-200"
                        >
                            <X size={18} />
                        </button>
                    </div>
                </div>

                {/* Content Area */}
                <div className="flex-1 overflow-hidden flex flex-col">
                    {mode === 'full' ? (
                        <div
                            ref={messagesContainerRef}
                            className="flex-1 overflow-y-auto px-5 py-2 space-y-4 scrollbar-hide"
                            onMouseDown={handleMouseDownOnMessages}
                            style={{ cursor: isDragging ? 'grabbing' : 'grab', scrollbarWidth: 'none', msOverflowStyle: 'none' }}
                        >
                            <style>{`
                                .scrollbar-hide::-webkit-scrollbar { display: none; }
                            `}</style>
                            {messages.length === 0 ? (
                                <div className="h-full flex flex-col items-center justify-center space-y-4 opacity-40">
                                    <div className="w-20 h-20 rounded-full bg-gray-100 flex items-center justify-center">
                                        <Inbox size={40} className="text-gray-300" />
                                    </div>
                                    <div className="text-center">
                                        <p className="text-gray-500 font-medium">暂无重要通知</p>
                                        <p className="text-xs text-gray-400 mt-1">当班级有新动态时，会在这里显示</p>
                                    </div>
                                </div>
                            ) : (
                                messages.map((msg, idx) => (
                                    <div key={msg.id} className="group animate-in fade-in slide-in-from-bottom-4 duration-300">
                                        <div className="flex items-center gap-2 mb-1.5 px-1">
                                            <div className="w-1.5 h-1.5 rounded-full bg-blue-500"></div>
                                            <span className="text-[11px] font-bold text-gray-400 uppercase tracking-wider">{formatTime(msg.timestamp)}</span>
                                            <div className="h-[1px] flex-1 bg-gradient-to-r from-gray-100 to-transparent"></div>
                                        </div>
                                        <div className="relative pl-3 border-l border-gray-100 ml-0.5 space-y-1">
                                            <div className="bg-white/60 hover:bg-white/80 border border-gray-100 rounded-2xl p-4 shadow-sm transition-all duration-300 group-hover:shadow-md group-hover:border-blue-100">
                                                <div className="flex items-start justify-between mb-2">
                                                    <div className="flex items-center gap-2">
                                                        <div className="w-6 h-6 rounded-lg bg-blue-50 text-blue-500 flex items-center justify-center">
                                                            <MessageSquare size={12} fill="currentColor" />
                                                        </div>
                                                        <span className="text-xs font-bold text-gray-700">{msg.senderName}</span>
                                                    </div>
                                                </div>
                                                <p className="text-gray-600 text-[13px] leading-relaxed break-words">
                                                    {msg.content}
                                                </p>
                                            </div>
                                        </div>
                                    </div>
                                ))
                            )}
                            <div ref={messagesEndRef} className="h-4" />
                        </div>
                    ) : (
                        /* Minimal Mode */
                        <div className="px-5 h-full flex items-center">
                            {latestMessage ? (
                                <div className="w-full flex items-center gap-4 bg-white/40 rounded-2xl p-2.5 pr-4 group cursor-pointer hover:bg-white/60 transition-all duration-300" onClick={toggleMode}>
                                    <div className="w-10 h-10 rounded-xl bg-blue-500 flex items-center justify-center shrink-0 shadow-lg shadow-blue-500/10">
                                        <Bell size={18} className="text-white animate-pulse" />
                                    </div>
                                    <div className="flex-1 min-w-0">
                                        <div className="flex items-center justify-between mb-0.5">
                                            <span className="text-xs font-bold text-gray-800 truncate">{latestMessage.senderName}</span>
                                            <span className="text-[10px] text-gray-400 font-medium">{formatTime(latestMessage.timestamp)}</span>
                                        </div>
                                        <p className="text-xs text-gray-500 truncate leading-relaxed">
                                            {latestMessage.content}
                                        </p>
                                    </div>
                                    <ChevronRight size={16} className="text-gray-300 group-hover:text-blue-400 transition-colors" />
                                </div>
                            ) : (
                                <div className="w-full flex items-center gap-3 text-gray-400 px-2">
                                    <Info size={16} />
                                    <span className="text-sm font-medium">当前没有未读通知</span>
                                </div>
                            )}
                        </div>
                    )}
                </div>

                {/* Footer Gradient Decor */}
                <div className="h-8 bg-gradient-to-t from-white/50 to-transparent pointer-events-none shrink-0" />
            </div>
        </div>
    );
};

export default ClassTextMessageWindow;
