import { useState, useEffect } from 'react';
import { X, Users, Check } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { createGroup, invalidateTIMGroupsCache } from '../../utils/tim';

interface CreateNormalGroupModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo?: any;
    onSuccess?: () => void;
    // friends prop removed, we fetch internally
}

interface FriendInfo {
    id: string;
    name: string;
    avatar: string;
}

const CreateNormalGroupModal = ({ isOpen, onClose, userInfo, onSuccess }: CreateNormalGroupModalProps) => {
    const [groupName, setGroupName] = useState('');
    const [isLoading, setIsLoading] = useState(false);
    const [selectedFriends, setSelectedFriends] = useState<string[]>([]);
    const [friendList, setFriendList] = useState<FriendInfo[]>([]);
    const [loadingFriends, setLoadingFriends] = useState(false);

    // Fetch friends when modal opens
    useEffect(() => {
        if (isOpen && userInfo?.id_number) {
            fetchFriends();
            setGroupName('');
            setSelectedFriends([]);
            setIsLoading(false);
        }
    }, [isOpen, userInfo]);

    const fetchFriends = async () => {
        setLoadingFriends(true);
        try {
            const token = localStorage.getItem('token');
            const resText = await invoke<string>('get_user_friends', {
                idCard: userInfo.id_number,
                token
            });
            const res = JSON.parse(resText);
            if (res.friends) {
                const mapped = res.friends.map((f: any) => ({
                    id: f.teacher_info?.teacher_unique_id || f.teacher_unique_id,
                    name: f.user_details?.name || f.teacher_info?.name || f.name || '未知好友',
                    avatar: f.user_details?.avatar || f.avatar || ''
                }));
                setFriendList(mapped);
            }
        } catch (e) {
            console.error('Failed to fetch friends:', e);
        } finally {
            setLoadingFriends(false);
        }
    };

    if (!isOpen) return null;

    const handleCreate = async () => {
        if (!groupName.trim()) {
            return;
        }

        setIsLoading(true);
        try {
            console.log('[CreateNormalGroup] Creating group via SDK:', groupName.trim());

            // Use SDK to create group with initial members
            const newGroup = await createGroup(groupName.trim(), 'Public', selectedFriends);

            console.log('[CreateNormalGroup] Group Created Successfully via SDK. GroupId:', newGroup.groupID);

            onSuccess?.();

            // Just one refresh is enough since SDK already knows about the group
            const { emit } = await import('@tauri-apps/api/event');
            invalidateTIMGroupsCache();
            await emit('refresh-class-list');

            onClose();
        } catch (error) {
            console.error("[CreateNormalGroup] Failed to create group:", error);
            alert(`创建失败: ${error}`);
        } finally {
            setIsLoading(false);
        }
    };

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div className="bg-white rounded-xl shadow-2xl w-[400px] flex flex-col overflow-hidden animate-in zoom-in-95 duration-200">

                {/* Header */}
                <div className="bg-gradient-to-r from-blue-500 to-blue-600 p-4 flex items-center justify-between text-white shadow-md">
                    <div className="flex items-center gap-2">
                        <Users className="w-5 h-5" />
                        <span className="font-bold text-lg">创建普通群</span>
                    </div>
                    <button
                        onClick={onClose}
                        className="text-white/80 hover:text-white hover:bg-white/20 rounded-full p-1 transition-colors"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-6 flex flex-col gap-6">
                    <div className="flex flex-col gap-2">
                        <label className="text-gray-700 font-medium text-sm">群组名称</label>
                        <input
                            type="text"
                            value={groupName}
                            onChange={(e) => setGroupName(e.target.value)}
                            placeholder="请输入群组名称..."
                            className="w-full p-3 rounded-lg border border-gray-300 bg-gray-50 text-gray-900 outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent transition-all placeholder:text-gray-400"
                            maxLength={30}
                            autoFocus
                        />
                        <p className="text-xs text-gray-500 text-right">{groupName.length}/30</p>
                    </div>

                    <div className="flex flex-col gap-2 flex-1 min-h-0">
                        <label className="text-gray-700 font-medium text-sm">邀请初始成员 ({selectedFriends.length})</label>
                        <div className="border border-gray-200 rounded-lg bg-gray-50 overflow-y-auto h-[200px] p-2">
                            {loadingFriends ? (
                                <div className="h-full flex items-center justify-center text-gray-400 text-sm">
                                    <div className="w-4 h-4 border-2 border-gray-300 border-t-blue-500 rounded-full animate-spin mr-2" />
                                    加载好友列表...
                                </div>
                            ) : friendList.length === 0 ? (
                                <div className="h-full flex flex-col items-center justify-center text-gray-400">
                                    <Users size={24} className="mb-2 opacity-50" />
                                    <span className="text-sm">暂无好友可邀请</span>
                                </div>
                            ) : (
                                <div className="space-y-1">
                                    {friendList.map(friend => {
                                        const isSelected = selectedFriends.includes(friend.id);
                                        return (
                                            <div
                                                key={friend.id}
                                                onClick={() => {
                                                    setSelectedFriends(prev =>
                                                        prev.includes(friend.id)
                                                            ? prev.filter(id => id !== friend.id)
                                                            : [...prev, friend.id]
                                                    );
                                                }}
                                                className={`flex items-center gap-3 p-2 rounded-lg cursor-pointer transition-all ${isSelected
                                                    ? 'bg-blue-50 border border-blue-200'
                                                    : 'hover:bg-white border border-transparent'
                                                    }`}
                                            >
                                                {friend.avatar ? (
                                                    <img src={friend.avatar} alt="" className="w-8 h-8 rounded-full object-cover" />
                                                ) : (
                                                    <div className="w-8 h-8 rounded-full bg-gradient-to-br from-blue-400 to-indigo-500 flex items-center justify-center text-white text-xs font-bold">
                                                        {friend.name.charAt(0)}
                                                    </div>
                                                )}
                                                <span className="flex-1 text-sm text-gray-700 font-medium">{friend.name}</span>
                                                <div className={`w-5 h-5 rounded-full border flex items-center justify-center transition-all ${isSelected
                                                    ? 'bg-blue-500 border-blue-500'
                                                    : 'border-gray-300 bg-white'
                                                    }`}>
                                                    {isSelected && <Check size={12} className="text-white" />}
                                                </div>
                                            </div>
                                        );
                                    })}
                                </div>
                            )}
                        </div>
                    </div>
                </div>


                {/* Footer */}
                <div className="p-4 bg-gray-50 border-t border-gray-100 flex justify-end gap-3">
                    <button
                        onClick={onClose}
                        className="px-5 py-2.5 text-gray-600 bg-white border border-gray-300 rounded-lg hover:bg-gray-50 font-medium transition-colors shadow-sm"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleCreate}
                        disabled={isLoading || !groupName.trim()}
                        className="px-5 py-2.5 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-lg hover:from-blue-600 hover:to-blue-700 font-medium shadow-md disabled:opacity-50 disabled:cursor-not-allowed flex items-center gap-2 transition-all"
                    >
                        {isLoading ? (
                            <>
                                <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin" />
                                <span>创建中...</span>
                            </>
                        ) : (
                            <span>确认创建</span>
                        )}
                    </button>
                </div>

            </div>
        </div >
    );
};

export default CreateNormalGroupModal;
