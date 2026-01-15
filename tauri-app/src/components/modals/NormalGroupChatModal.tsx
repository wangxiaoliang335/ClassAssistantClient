import { useState, useEffect, useRef, useCallback } from 'react';
import { createPortal } from 'react-dom';
import { X, Send, Image, Paperclip, Users, Settings, UserPlus, LogOut, Crown, Shield, Check } from 'lucide-react';
import { tim, sendMessage, sendImageMessage, sendFileMessage, addMessageListener, getGroupMemberList, quitGroup, dismissGroup, addGroupMember } from '../../utils/tim';
import TencentCloudChat from '@tencentcloud/chat';

interface NormalGroupChatModalProps {
    isOpen: boolean;
    onClose: () => void;
    groupId: string;
    groupName: string;
    isOwner: boolean;
    userInfo?: any;
    friends?: FriendInfo[];
}

interface FriendInfo {
    teacher_unique_id: string;
    name: string;
    avatar?: string;
}

interface ChatMessage {
    id: string;
    text: string;
    senderName: string;
    senderAvatar: string;
    isMine: boolean;
    time: Date;
    type: 'text' | 'image' | 'file';
    imageUrl?: string;
    fileName?: string;
    fileUrl?: string;
}

interface GroupMember {
    userID: string;
    nick: string;
    avatar: string;
    role: string; // Owner, Admin, Member
}

const NormalGroupChatModal = ({ isOpen, onClose, groupId, groupName, isOwner, userInfo, friends = [] }: NormalGroupChatModalProps) => {
    const [messages, setMessages] = useState<ChatMessage[]>([]);
    const [inputText, setInputText] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const [showGroupInfo, setShowGroupInfo] = useState(false);
    const [members, setMembers] = useState<GroupMember[]>([]);
    const [loadingMembers, setLoadingMembers] = useState(false);

    // Add member dialog state
    const [showAddMember, setShowAddMember] = useState(false);
    const [selectedFriends, setSelectedFriends] = useState<string[]>([]);

    // Draggable state
    const [position, setPosition] = useState({ x: 0, y: 0 });
    const [isDragging, setIsDragging] = useState(false);
    const dragOffset = useRef({ x: 0, y: 0 });
    const modalRef = useRef<HTMLDivElement>(null);

    const messagesEndRef = useRef<HTMLDivElement>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const imageInputRef = useRef<HTMLInputElement>(null);

    // Get current user ID
    const currentUserId = userInfo?.teacher_unique_id || userInfo?.id || localStorage.getItem('unique_id') || '';

    // Center modal on open
    useEffect(() => {
        if (isOpen) {
            setPosition({ x: 0, y: 0 });
        }
    }, [isOpen]);

    // Scroll to bottom when messages change
    useEffect(() => {
        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }, [messages]);

    // Load message history when modal opens
    useEffect(() => {
        if (isOpen && groupId) {
            loadMessageHistory();
        }
    }, [isOpen, groupId]);

    // Listen for incoming messages
    useEffect(() => {
        if (!isOpen || !groupId) return;

        const removeListener = addMessageListener((event: any) => {
            const messageList = event.data || [];
            messageList.forEach((msg: any) => {
                if (msg.conversationID === `GROUP${groupId}`) {
                    const newMsg = parseMessage(msg);
                    if (newMsg) {
                        setMessages(prev => {
                            if (prev.some(m => m.id === newMsg.id)) return prev;
                            return [...prev, newMsg];
                        });
                    }
                }
            });
        });

        return () => removeListener();
    }, [isOpen, groupId, currentUserId]);

    // Drag handlers
    const handleMouseDown = useCallback((e: React.MouseEvent) => {
        if ((e.target as HTMLElement).closest('.no-drag')) return;
        setIsDragging(true);
        const rect = modalRef.current?.getBoundingClientRect();
        if (rect) {
            dragOffset.current = {
                x: e.clientX - rect.left - rect.width / 2,
                y: e.clientY - rect.top - rect.height / 2
            };
        }
    }, []);

    useEffect(() => {
        if (!isDragging) return;

        const handleMouseMove = (e: MouseEvent) => {
            const newX = e.clientX - window.innerWidth / 2 - dragOffset.current.x;
            const newY = e.clientY - window.innerHeight / 2 - dragOffset.current.y;
            setPosition({ x: newX, y: newY });
        };

        const handleMouseUp = () => setIsDragging(false);

        document.addEventListener('mousemove', handleMouseMove);
        document.addEventListener('mouseup', handleMouseUp);
        return () => {
            document.removeEventListener('mousemove', handleMouseMove);
            document.removeEventListener('mouseup', handleMouseUp);
        };
    }, [isDragging]);

    const parseMessage = (msg: any): ChatMessage | null => {
        const isMine = msg.from === currentUserId;
        const baseMsg = {
            id: msg.ID || msg.clientSequence || `${Date.now()}-${Math.random()}`,
            senderName: msg.nick || msg.from || '未知用户',
            senderAvatar: msg.avatar || '',
            isMine,
            time: new Date(msg.time * 1000 || Date.now()),
        };

        if (msg.type === TencentCloudChat.TYPES.MSG_TEXT) {
            return { ...baseMsg, type: 'text', text: msg.payload?.text || '' };
        } else if (msg.type === TencentCloudChat.TYPES.MSG_IMAGE) {
            return {
                ...baseMsg,
                type: 'image',
                text: '[图片]',
                imageUrl: msg.payload?.imageInfoArray?.[0]?.url || msg.payload?.imageInfoArray?.[1]?.url || ''
            };
        } else if (msg.type === TencentCloudChat.TYPES.MSG_FILE) {
            return {
                ...baseMsg,
                type: 'file',
                text: `[文件] ${msg.payload?.fileName || ''}`,
                fileName: msg.payload?.fileName || '文件',
                fileUrl: msg.payload?.fileUrl || ''
            };
        }
        return null;
    };

    const loadMessageHistory = async () => {
        setIsLoading(true);
        try {
            const res = await tim.getMessageList({
                conversationID: `GROUP${groupId}`
            });
            if (res.data?.messageList) {
                const parsedMessages = res.data.messageList
                    .map(parseMessage)
                    .filter((m: ChatMessage | null): m is ChatMessage => m !== null);
                setMessages(parsedMessages);
            }
        } catch (error) {
            console.error('Failed to load message history:', error);
        } finally {
            setIsLoading(false);
        }
    };

    const loadGroupMembers = async () => {
        setLoadingMembers(true);
        try {
            const memberList = await getGroupMemberList(groupId);
            setMembers(memberList.map((m: any) => ({
                userID: m.userID,
                nick: m.nick || m.userID,
                avatar: m.avatar || '',
                role: m.role || 'Member'
            })));
        } catch (error) {
            console.error('Failed to load group members:', error);
        } finally {
            setLoadingMembers(false);
        }
    };

    const handleSendText = async () => {
        if (!inputText.trim() || !groupId) return;
        const text = inputText.trim();
        setInputText('');

        try {
            const sentMsg = await sendMessage(groupId, text, 'GROUP');
            if (sentMsg) {
                const newMsg = parseMessage(sentMsg);
                if (newMsg) setMessages(prev => [...prev, newMsg]);
            }
        } catch (error) {
            console.error('Failed to send message:', error);
            alert('发送消息失败');
        }
    };

    const handleSendImage = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !groupId) return;

        try {
            const sentMsg = await sendImageMessage(groupId, file, 'GROUP');
            if (sentMsg) {
                const newMsg = parseMessage(sentMsg);
                if (newMsg) setMessages(prev => [...prev, newMsg]);
            }
        } catch (error) {
            console.error('Failed to send image:', error);
            alert('发送图片失败');
        }
        if (imageInputRef.current) imageInputRef.current.value = '';
    };

    const handleSendFile = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !groupId) return;

        try {
            const sentMsg = await sendFileMessage(groupId, file, 'GROUP');
            if (sentMsg) {
                const newMsg = parseMessage(sentMsg);
                if (newMsg) setMessages(prev => [...prev, newMsg]);
            }
        } catch (error) {
            console.error('Failed to send file:', error);
            alert('发送文件失败');
        }
        if (fileInputRef.current) fileInputRef.current.value = '';
    };

    const handleKeyDown = (e: React.KeyboardEvent) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleSendText();
        }
    };

    const handleToggleGroupInfo = () => {
        if (!showGroupInfo && members.length === 0) {
            loadGroupMembers();
        }
        setShowGroupInfo(!showGroupInfo);
    };

    const handleQuitGroup = async () => {
        if (!confirm(isOwner ? '确定要解散此群吗？此操作不可恢复。' : '确定要退出此群吗？')) return;

        try {
            if (isOwner) {
                await dismissGroup(groupId);
                alert('群已解散');
            } else {
                await quitGroup(groupId);
                alert('已退出群聊');
            }
            onClose();
        } catch (error) {
            console.error('Failed to quit/dismiss group:', error);
            alert(isOwner ? '解散群失败' : '退出群失败');
        }
    };

    const handleAddMember = async () => {
        if (selectedFriends.length === 0) return;
        try {
            await addGroupMember(groupId, selectedFriends);
            alert(`已邀请 ${selectedFriends.length} 位好友`);
            setSelectedFriends([]);
            setShowAddMember(false);
            loadGroupMembers(); // Refresh member list
        } catch (error: any) {
            console.error('Failed to add member:', error);
            alert(`添加成员失败: ${error?.message || '未知错误'}`);
        }
    };

    const toggleFriendSelection = (friendId: string) => {
        setSelectedFriends(prev =>
            prev.includes(friendId)
                ? prev.filter(id => id !== friendId)
                : [...prev, friendId]
        );
    };

    // Filter out friends who are already members
    const availableFriends = friends.filter(
        f => !members.some(m => m.userID === f.teacher_unique_id)
    );

    const getRoleIcon = (role: string) => {
        if (role === 'Owner') return <Crown size={14} className="text-amber-500" />;
        if (role === 'Admin') return <Shield size={14} className="text-blue-500" />;
        return null;
    };

    if (!isOpen) return null;

    return createPortal(
        <div className="fixed inset-0 z-[9999] pointer-events-none">
            <div
                ref={modalRef}
                style={{
                    position: 'fixed',
                    left: `calc(50% + ${position.x}px)`,
                    top: `calc(50% + ${position.y}px)`,
                    transform: 'translate(-50%, -50%)'
                }}
                className={`bg-white rounded-2xl shadow-2xl flex overflow-hidden pointer-events-auto ${isDragging ? 'shadow-3xl cursor-grabbing' : ''}`}
            >
                {/* Main Chat Area */}
                <div className="w-[550px] h-[650px] flex flex-col">
                    {/* Header - Draggable */}
                    <div
                        onMouseDown={handleMouseDown}
                        className="flex items-center justify-between px-4 py-3 border-b border-gray-100 bg-gradient-to-r from-indigo-500 via-purple-500 to-pink-500 cursor-grab active:cursor-grabbing select-none"
                    >
                        <div className="flex items-center gap-3">
                            <div className="w-10 h-10 rounded-xl bg-white/20 backdrop-blur flex items-center justify-center text-white shadow-lg">
                                <Users size={20} />
                            </div>
                            <div>
                                <h2 className="text-base font-bold text-white drop-shadow">{groupName || '群聊'}</h2>
                                <p className="text-xs text-white/80">{members.length > 0 ? `${members.length} 位成员` : (isOwner ? '群主' : '成员')}</p>
                            </div>
                        </div>
                        <div className="flex items-center gap-1 no-drag">
                            <button
                                onClick={handleToggleGroupInfo}
                                className={`p-2 rounded-lg transition-all ${showGroupInfo ? 'bg-white/30 text-white' : 'text-white/80 hover:bg-white/20 hover:text-white'}`}
                                title="群信息"
                            >
                                <Settings size={18} />
                            </button>
                            <button
                                onClick={onClose}
                                className="p-2 text-white/80 hover:bg-white/20 hover:text-white rounded-lg transition-all"
                                title="关闭"
                            >
                                <X size={18} />
                            </button>
                        </div>
                    </div>

                    {/* Messages Area */}
                    <div className="flex-1 overflow-y-auto p-4 space-y-3 bg-gradient-to-b from-gray-50 to-white">
                        {isLoading && (
                            <div className="text-center text-gray-400 py-4">
                                <div className="inline-block w-5 h-5 border-2 border-gray-300 border-t-indigo-500 rounded-full animate-spin mb-2"></div>
                                <p>加载中...</p>
                            </div>
                        )}
                        {!isLoading && messages.length === 0 && (
                            <div className="text-center text-gray-400 py-8">
                                <Users size={40} className="mx-auto mb-2 text-gray-300" />
                                <p>暂无消息，发送第一条消息开始聊天吧！</p>
                            </div>
                        )}
                        {messages.map((msg) => (
                            <div key={msg.id} className={`flex ${msg.isMine ? 'justify-end' : 'justify-start'}`}>
                                <div className={`flex gap-2 max-w-[75%] ${msg.isMine ? 'flex-row-reverse' : ''}`}>
                                    <div className="flex-shrink-0">
                                        {msg.senderAvatar ? (
                                            <img src={msg.senderAvatar} alt="" className="w-9 h-9 rounded-xl object-cover shadow" />
                                        ) : (
                                            <div className={`w-9 h-9 rounded-xl flex items-center justify-center text-white text-sm font-medium shadow ${msg.isMine ? 'bg-gradient-to-br from-indigo-500 to-purple-600' : 'bg-gradient-to-br from-gray-400 to-gray-500'}`}>
                                                {msg.senderName.charAt(0)}
                                            </div>
                                        )}
                                    </div>
                                    <div className={`flex flex-col ${msg.isMine ? 'items-end' : 'items-start'}`}>
                                        {!msg.isMine && <span className="text-xs text-gray-400 mb-1 ml-1">{msg.senderName}</span>}
                                        <div className={`rounded-2xl px-4 py-2.5 ${msg.isMine ? 'bg-gradient-to-r from-indigo-500 to-purple-600 text-white rounded-br-md shadow-lg' : 'bg-white text-gray-800 shadow-md rounded-bl-md border border-gray-100'}`}>
                                            {msg.type === 'image' && msg.imageUrl ? (
                                                <img src={msg.imageUrl} alt="图片" className="max-w-[280px] rounded-lg" style={{ maxHeight: 200 }} />
                                            ) : msg.type === 'file' ? (
                                                <a href={msg.fileUrl} target="_blank" rel="noopener noreferrer" className={`flex items-center gap-2 ${msg.isMine ? 'text-white/90' : 'text-indigo-600'}`}>
                                                    <Paperclip size={16} /> {msg.fileName}
                                                </a>
                                            ) : (
                                                <p className="whitespace-pre-wrap break-words text-sm leading-relaxed">{msg.text}</p>
                                            )}
                                        </div>
                                        <span className="text-[10px] text-gray-300 mt-1 mx-1">
                                            {msg.time.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })}
                                        </span>
                                    </div>
                                </div>
                            </div>
                        ))}
                        <div ref={messagesEndRef} />
                    </div>

                    {/* Input Area */}
                    <div className="border-t border-gray-100 p-3 bg-white">
                        <div className="flex items-center gap-2">
                            <input type="file" ref={imageInputRef} onChange={handleSendImage} accept="image/*" className="hidden" />
                            <button onClick={() => imageInputRef.current?.click()} className="p-2.5 text-gray-400 hover:text-indigo-500 hover:bg-indigo-50 rounded-xl transition-all" title="发送图片">
                                <Image size={20} />
                            </button>
                            <input type="file" ref={fileInputRef} onChange={handleSendFile} className="hidden" />
                            <button onClick={() => fileInputRef.current?.click()} className="p-2.5 text-gray-400 hover:text-indigo-500 hover:bg-indigo-50 rounded-xl transition-all" title="发送文件">
                                <Paperclip size={20} />
                            </button>
                            <input
                                type="text"
                                value={inputText}
                                onChange={(e) => setInputText(e.target.value)}
                                onKeyDown={handleKeyDown}
                                placeholder="输入消息..."
                                className="flex-1 px-4 py-2.5 bg-gray-50 border border-gray-200 rounded-xl text-sm focus:outline-none focus:border-indigo-400 focus:ring-2 focus:ring-indigo-100 transition-all placeholder:text-gray-400"
                            />
                            <button
                                onClick={handleSendText}
                                disabled={!inputText.trim()}
                                className="p-2.5 bg-gradient-to-r from-indigo-500 to-purple-600 text-white rounded-xl hover:from-indigo-600 hover:to-purple-700 disabled:opacity-40 disabled:cursor-not-allowed transition-all shadow-md hover:shadow-lg disabled:shadow-none"
                            >
                                <Send size={20} />
                            </button>
                        </div>
                    </div>
                </div>

                {/* Group Info Panel */}
                {showGroupInfo && (
                    <div className="w-[240px] h-[650px] border-l border-gray-100 bg-gray-50/80 flex flex-col">
                        <div className="p-4 border-b border-gray-100 bg-white">
                            <h3 className="font-semibold text-gray-800">群信息</h3>
                            <p className="text-xs text-gray-400 mt-1">{groupId}</p>
                        </div>

                        {/* Members List */}
                        <div className="flex-1 overflow-y-auto">
                            <div className="p-3">
                                <div className="flex items-center justify-between mb-3">
                                    <span className="text-xs font-medium text-gray-500 uppercase tracking-wider">成员 ({members.length})</span>
                                    {isOwner && (
                                        <button
                                            onClick={() => setShowAddMember(true)}
                                            className="p-1.5 text-indigo-500 hover:bg-indigo-50 rounded-lg transition-colors"
                                            title="邀请成员"
                                        >
                                            <UserPlus size={16} />
                                        </button>
                                    )}
                                </div>

                                {loadingMembers ? (
                                    <div className="text-center py-4 text-gray-400 text-sm">加载中...</div>
                                ) : (
                                    <div className="space-y-1">
                                        {members.map((member) => (
                                            <div key={member.userID} className="flex items-center gap-2 p-2 rounded-lg hover:bg-white transition-colors group">
                                                {member.avatar ? (
                                                    <img src={member.avatar} alt="" className="w-8 h-8 rounded-lg object-cover" />
                                                ) : (
                                                    <div className="w-8 h-8 rounded-lg bg-gradient-to-br from-indigo-400 to-purple-500 flex items-center justify-center text-white text-xs font-medium">
                                                        {member.nick.charAt(0)}
                                                    </div>
                                                )}
                                                <div className="flex-1 min-w-0">
                                                    <div className="flex items-center gap-1">
                                                        <span className="text-sm text-gray-700 truncate">{member.nick}</span>
                                                        {getRoleIcon(member.role)}
                                                    </div>
                                                    {member.userID === currentUserId && (
                                                        <span className="text-[10px] text-indigo-500">我</span>
                                                    )}
                                                </div>
                                            </div>
                                        ))}
                                    </div>
                                )}
                            </div>
                        </div>

                        {/* Actions */}
                        <div className="p-3 border-t border-gray-100 bg-white">
                            <button
                                onClick={handleQuitGroup}
                                className="w-full flex items-center justify-center gap-2 px-4 py-2.5 text-red-500 hover:bg-red-50 rounded-xl transition-colors font-medium text-sm"
                            >
                                <LogOut size={16} />
                                {isOwner ? '解散群聊' : '退出群聊'}
                            </button>
                        </div>
                    </div>
                )}
            </div>

            {/* Add Member Dialog */}
            {showAddMember && (
                <div className="fixed inset-0 z-[10000] flex items-center justify-center bg-black/40 pointer-events-auto">
                    <div className="bg-white rounded-xl shadow-2xl w-[360px] max-h-[500px] flex flex-col animate-in zoom-in-95">
                        <div className="p-4 border-b border-gray-100 flex items-center justify-between">
                            <div>
                                <h3 className="text-lg font-semibold text-gray-800">邀请好友加入群聊</h3>
                                <p className="text-xs text-gray-400 mt-1">已选择 {selectedFriends.length} 位好友</p>
                            </div>
                            <button
                                onClick={() => { setShowAddMember(false); setSelectedFriends([]); }}
                                className="p-1.5 text-gray-400 hover:bg-gray-100 hover:text-gray-600 rounded-lg transition-colors"
                            >
                                <X size={18} />
                            </button>
                        </div>

                        <div className="flex-1 overflow-y-auto p-3 max-h-[300px]">
                            {availableFriends.length === 0 ? (
                                <div className="text-center py-8 text-gray-400">
                                    <Users size={32} className="mx-auto mb-2 text-gray-300" />
                                    <p>没有可邀请的好友</p>
                                </div>
                            ) : (
                                <div className="space-y-1">
                                    {availableFriends.map(friend => {
                                        const isSelected = selectedFriends.includes(friend.teacher_unique_id);
                                        return (
                                            <div
                                                key={friend.teacher_unique_id}
                                                onClick={() => toggleFriendSelection(friend.teacher_unique_id)}
                                                className={`flex items-center gap-3 p-2.5 rounded-lg cursor-pointer transition-all ${isSelected
                                                    ? 'bg-indigo-50 ring-2 ring-indigo-400'
                                                    : 'hover:bg-gray-50'
                                                    }`}
                                            >
                                                {friend.avatar ? (
                                                    <img src={friend.avatar} alt="" className="w-9 h-9 rounded-lg object-cover" />
                                                ) : (
                                                    <div className="w-9 h-9 rounded-lg bg-gradient-to-br from-indigo-400 to-purple-500 flex items-center justify-center text-white text-sm font-medium">
                                                        {friend.name.charAt(0)}
                                                    </div>
                                                )}
                                                <span className="flex-1 text-sm text-gray-700 font-medium">{friend.name}</span>
                                                <div className={`w-5 h-5 rounded-full border-2 flex items-center justify-center transition-all ${isSelected
                                                    ? 'bg-indigo-500 border-indigo-500'
                                                    : 'border-gray-300'
                                                    }`}>
                                                    {isSelected && <Check size={12} className="text-white" />}
                                                </div>
                                            </div>
                                        );
                                    })}
                                </div>
                            )}
                        </div>

                        <div className="p-3 border-t border-gray-100 flex gap-2">
                            <button
                                onClick={() => { setShowAddMember(false); setSelectedFriends([]); }}
                                className="flex-1 px-4 py-2.5 text-gray-600 hover:bg-gray-100 rounded-lg transition-colors font-medium"
                            >
                                取消
                            </button>
                            <button
                                onClick={handleAddMember}
                                disabled={selectedFriends.length === 0}
                                className="flex-1 px-4 py-2.5 bg-indigo-500 text-white rounded-lg hover:bg-indigo-600 disabled:opacity-50 disabled:cursor-not-allowed transition-colors font-medium"
                            >
                                邀请 ({selectedFriends.length})
                            </button>
                        </div>
                    </div>
                </div>
            )}
        </div>,
        document.body
    );
};

export default NormalGroupChatModal;
