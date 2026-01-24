import TencentCloudChat from '@tencentcloud/chat';
import TIMUploadPlugin from 'tim-upload-plugin';

const SDKAppID = 1600111046;

export const tim = TencentCloudChat.create({
    SDKAppID
});

// Register Upload Plugin
tim.registerPlugin({ 'tim-upload-plugin': TIMUploadPlugin });

// Set log level
tim.setLogLevel(1);

export type MessageListener = (event: any) => void;
const messageListeners: MessageListener[] = [];

export const addMessageListener = (listener: MessageListener) => {
    messageListeners.push(listener);
    return () => {
        const index = messageListeners.indexOf(listener);
        if (index > -1) {
            messageListeners.splice(index, 1);
        }
    };
};

tim.on(TencentCloudChat.EVENT.MESSAGE_RECEIVED, (event: any) => {
    messageListeners.forEach(listener => listener(event));
});

export let isSDKReady = false;

tim.on(TencentCloudChat.EVENT.SDK_READY, () => {
    isSDKReady = true;
});

tim.on(TencentCloudChat.EVENT.SDK_NOT_READY, () => {
    isSDKReady = false;
});

tim.on(TencentCloudChat.EVENT.KICKED_OUT, () => {
    isSDKReady = false;
});

export const loginTIM = (userID: string, userSig: string) => {
    return new Promise((resolve) => {
        if (isSDKReady) {
            resolve(true);
            return;
        }


        // 5s timeout to prevent hanging
        const timeout = setTimeout(() => {
            tim.off(TencentCloudChat.EVENT.SDK_READY, onReady);
            resolve(false);
        }, 5000);

        const onReady = () => {
            clearTimeout(timeout);
            tim.off(TencentCloudChat.EVENT.SDK_READY, onReady);
            resolve(true);
        };

        tim.on(TencentCloudChat.EVENT.SDK_READY, onReady);

        tim.login({ userID, userSig }).then(() => {
        }).catch((error: any) => {
            // Error 6014: Repeat Login.
            if (error?.code !== 6014) {
                console.error('TIM login() failed:', error);
            }
        });
    });
};

// Helper function to wait for SDK ready
const waitForSDKReady = (timeoutMs: number = 5000): Promise<boolean> => {
    return new Promise((resolve) => {
        if (isSDKReady) {
            resolve(true);
            return;
        }

        const startTime = Date.now();
        const checkInterval = setInterval(() => {
            if (isSDKReady) {
                clearInterval(checkInterval);
                resolve(true);
            } else if (Date.now() - startTime > timeoutMs) {
                clearInterval(checkInterval);
                console.warn('TIM SDK ready timeout after', timeoutMs, 'ms');
                resolve(false);
            }
        }, 100);
    });
};

// Cache for TIM groups - used when SDK isn't ready yet but groups were fetched earlier
let cachedTIMGroups: any[] = [];

export const getTIMGroups = async () => {
    // If SDK is ready, fetch fresh data
    if (isSDKReady) {
        try {
            const res = await tim.getGroupList();
            cachedTIMGroups = res.data.groupList;
            return res.data.groupList;
        } catch (error) {
            if (cachedTIMGroups.length > 0) {
                return cachedTIMGroups;
            }
            return [];
        }
    }

    const ready = await waitForSDKReady(5000);

    if (ready) {
        try {
            const { code, data } = await tim.getGroupList();
            if (code === 0) {
                cachedTIMGroups = data.groupList;
                return data.groupList;
            }
        } catch (error) {
            return cachedTIMGroups.length > 0 ? cachedTIMGroups : [];
        }
    }

    // SDK still not ready - return cached data if available
    if (cachedTIMGroups.length > 0) {
        return cachedTIMGroups;
    }

    return [];
};

// Export function to update cache from outside (e.g., ClassManagement)
export const setCachedTIMGroups = (groups: any[]) => {
    cachedTIMGroups = groups;
};

// Export function to invalidate/clear the cache (force fresh fetch on next call)
export const invalidateTIMGroupsCache = () => {
    cachedTIMGroups = [];
};

export const sendMessage = async (to: string, text: string, type: 'C2C' | 'GROUP' = 'GROUP') => {
    if (!isSDKReady) {
        console.error('sendMessage failed: SDK not ready');
        return null;
    }
    try {
        const message = tim.createTextMessage({
            to: to,
            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
            payload: {
                text: text
            }
        });
        const res = await tim.sendMessage(message);
        console.log('TIM sendMessage success:', res);
        return res.data.message; // Return the message object for UI update
    } catch (error) {
        console.error('TIM sendMessage failed:', error);
        throw error;
    }
};

export const getMessageList = async (groupID: string) => {
    if (!isSDKReady) return [];
    try {
        const res = await tim.getMessageList({ conversationID: `GROUP${groupID}` });
        console.log('TIM getMessageList success:', res.data.messageList.length);
        return res.data.messageList;
    } catch (error) {
        console.error('TIM getMessageList failed:', error);
        return [];
    }
};

export const getGroupMemberList = async (groupID: string) => {
    if (!isSDKReady) return [];
    try {
        const res = await tim.getGroupMemberList({ groupID, count: 30, offset: 0 });
        console.log('TIM getGroupMemberList success:', res.data.memberList.length);
        return res.data.memberList;
    } catch (error) {
        console.error('TIM getGroupMemberList failed:', error);
        return [];
    }
};

export const sendImageMessage = async (to: string, file: HTMLInputElement | File, type: 'C2C' | 'GROUP' = 'GROUP') => {
    if (!isSDKReady) {
        console.error('sendImageMessage failed: SDK not ready');
        return null;
    }
    try {
        const message = tim.createImageMessage({
            to: to,
            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
            payload: {
                file: file
            },
            onProgress: (percent: number) => {
                console.log('Image upload progress:', percent);
            }
        });
        const res = await tim.sendMessage(message);
        console.log('TIM sendImageMessage success:', res);
        return res.data.message;
    } catch (error) {
        console.error('TIM sendImageMessage failed:', error);
        throw error;
    }
};

export const sendFileMessage = async (to: string, file: HTMLInputElement | File, type: 'C2C' | 'GROUP' = 'GROUP') => {
    if (!isSDKReady) {
        console.error('sendFileMessage failed: SDK not ready');
        return null;
    }
    try {
        const message = tim.createFileMessage({
            to: to,
            conversationType: type === 'GROUP' ? TencentCloudChat.TYPES.CONV_GROUP : TencentCloudChat.TYPES.CONV_C2C,
            payload: {
                file: file
            },
            onProgress: (percent: number) => {
                console.log('File upload progress:', percent);
            }
        });
        const res = await tim.sendMessage(message);
        console.log('TIM sendFileMessage success:', res);
        return res.data.message;
    } catch (error) {
        console.error('TIM sendFileMessage failed:', error);
        throw error;
    }
};

export const dismissGroup = async (groupID: string) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        const res = await tim.dismissGroup(groupID);
        console.log('TIM dismissGroup success:', res);
        return res;
    } catch (error) {
        console.error('TIM dismissGroup failed:', error);
        throw error;
    }
};

export const quitGroup = async (groupID: string) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        const res = await tim.quitGroup(groupID);
        console.log('TIM quitGroup success:', res);
        return res;
    } catch (error) {
        console.error('TIM quitGroup failed:', error);
        throw error;
    }
};

export const changeGroupOwner = async (groupID: string, newOwnerID: string) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        const res = await tim.changeGroupOwner({
            groupID: groupID,
            newOwnerID: newOwnerID
        });
        console.log('TIM changeGroupOwner success:', res);
        return res;
    } catch (error) {
        console.error('TIM changeGroupOwner failed:', error);
        throw error;
    }
};

// Add members to a group
export const addGroupMember = async (groupID: string, userIDList: string[]) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        const res = await tim.addGroupMember({
            groupID: groupID,
            userIDList: userIDList
        });
        console.log('TIM addGroupMember success:', res);
        return res;
    } catch (error) {
        console.error('TIM addGroupMember failed:', error);
        throw error;
    }
};

export const checkGroupsExist = async (groupIDs: string[]): Promise<string[]> => {
    if (!isSDKReady || groupIDs.length === 0) return [];

    const existingGroups: string[] = [];

    // Helper to delay between requests
    const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

    // Check groups sequentially with throttling to avoid rate limits
    for (const groupID of groupIDs) {
        try {
            const res = await tim.getGroupProfile({ groupID });
            // If we get here without error, the group exists
            if (res.data.group && res.data.group.groupID) {
                existingGroups.push(res.data.group.groupID);
            }
        } catch (error: any) {
            // Error code 10010 means group doesn't exist - this is expected
            // Error code 10007 means "only group member can get group info" - group EXISTS but we're not a member
            if (error?.code === 10007) {
                console.log(`TIM checkGroupExist for ${groupID}: exists (not a member, code 10007)`);
                existingGroups.push(groupID);
            } else if (error?.code === 2996) {
                // Rate limit hit - wait longer and retry once
                console.log(`TIM checkGroupExist for ${groupID}: rate limited, waiting and retrying...`);
                await delay(500);
                try {
                    const retryRes = await tim.getGroupProfile({ groupID });
                    if (retryRes.data.group && retryRes.data.group.groupID) {
                        existingGroups.push(retryRes.data.group.groupID);
                    }
                } catch (retryError: any) {
                    if (retryError?.code === 10007) {
                        existingGroups.push(groupID);
                    } else if (retryError?.code !== 10010) {
                        console.log(`TIM checkGroupExist retry for ${groupID}:`, retryError?.code || retryError?.message);
                    }
                }
            } else if (error?.code !== 10010) {
                console.log(`TIM checkGroupExist for ${groupID}:`, error?.code || error?.message || 'unknown error');
            }
        }
        // Throttle: wait 100ms between requests
        await delay(100);
    }

    console.log('TIM checkGroupsExist: found', existingGroups.length, 'existing groups out of', groupIDs.length);
    return existingGroups;
};

// Create a group using SDK (automatically syncs to local cache)
// Create a group using SDK (automatically syncs to local cache)
export const createGroup = async (groupName: string, groupType: 'Public' | 'Private' | 'ChatRoom' | 'AVChatRoom' = 'Public', memberList: string[] = []) => {
    if (!isSDKReady) throw new Error('SDK not ready');
    try {
        // Determine SDK type constant based on groupType
        let sdkType = TencentCloudChat.TYPES.GRP_PUBLIC;
        if (groupType === 'Private') {
            sdkType = TencentCloudChat.TYPES.GRP_WORK; // Work is the new name for Private
        } else if (groupType === 'ChatRoom') {
            sdkType = TencentCloudChat.TYPES.GRP_MEETING;
        } else if (groupType === 'AVChatRoom') {
            sdkType = TencentCloudChat.TYPES.GRP_AVCHATROOM;
        }

        const payload: any = {
            type: sdkType,
            name: groupName,
            // For Public groups, allow free access
            joinOption: groupType === 'Public' ? TencentCloudChat.TYPES.JOIN_OPTIONS_FREE_ACCESS : undefined
        };

        if (memberList && memberList.length > 0) {
            payload.memberList = memberList.map(uid => ({ userID: uid }));
        }

        const res = await tim.createGroup(payload);

        console.log('TIM createGroup success:', res);

        // Invalidate cache so next getGroupList fetches fresh data
        invalidateTIMGroupsCache();

        return res.data.group;
    } catch (error) {
        console.error('TIM createGroup failed:', error);
        throw error;
    }
};
