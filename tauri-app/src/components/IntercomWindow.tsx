import { useEffect, useRef, useState } from 'react';
import { useParams } from 'react-router-dom';
import { invoke } from '@tauri-apps/api/core';
import { Mic, MicOff, X, Terminal, Users, Radio, Volume2, VolumeX } from 'lucide-react';

// Types
interface Member {
    id: string | number;
    name: string;
    avatar?: string;
}

interface RoomInfo {
    room_id: string;
    whip_url: string;
    whep_url: string;
    stream_name: string;
    group_id: string;
}

const IntercomWindow = () => {
    const { groupId } = useParams<{ groupId: string }>();
    const [statusLogs, setStatusLogs] = useState<{ msg: string; type: 'info' | 'success' | 'date' | 'error' | 'warn' }[]>([]);
    const [members, setMembers] = useState<Member[]>([]);
    const [isPublishing, setIsPublishing] = useState(false);
    const [intercomEnabled, setIntercomEnabled] = useState(false); // Can talk?
    const [roomInfo, setRoomInfoState] = useState<RoomInfo | null>(null);
    const [speakingUser, setSpeakingUser] = useState<string | null>(null);
    const [isWsConnected, setIsWsConnected] = useState(false);
    const [isMuted, setIsMuted] = useState(false);
    const [myId, setMyIdState] = useState<string | null>(null);

    // Refs for WebRTC and WS
    const wsRef = useRef<WebSocket | null>(null);
    const publishPcRef = useRef<RTCPeerConnection | null>(null);
    const subscribePcRef = useRef<RTCPeerConnection | null>(null);
    const publishStreamRef = useRef<MediaStream | null>(null);
    const statusLogRef = useRef<HTMLDivElement>(null);
    const isTalkingRef = useRef(false); // Track active talking intent to handle async races
    const roomInfoRef = useRef<RoomInfo | null>(null);
    const myIdRef = useRef<string | null>(null);

    const log = (msg: string, type: 'info' | 'success' | 'date' | 'error' | 'warn' = 'info') => {
        const time = new Date().toLocaleTimeString('zh-CN', { hour12: false });
        console.log(`[Intercom-Log] ${time} ${msg}`); // Also log to browser console
        setStatusLogs(prev => [...prev, { msg: `${time} ${msg}`, type }]);
        if (statusLogRef.current) {
            statusLogRef.current.scrollTop = statusLogRef.current.scrollHeight;
        }
    };

    const setRoomInfo = (info: RoomInfo | null) => {
        roomInfoRef.current = info;
        setRoomInfoState(info);
    };

    const setMyId = (id: string | null) => {
        myIdRef.current = id;
        setMyIdState(id);
    };

    // Initialize: Fetch Data -> Connect WS -> (Wait for room) -> Pull Stream
    useEffect(() => {
        const init = async () => {
            if (!groupId) return;

            // 1. Get User Info
            try {
                const uStr = localStorage.getItem('user_info');
                if (uStr) {
                    JSON.parse(uStr);
                }
            } catch (err) { console.error(err); }

            // 2. Fetch Group Members & Temp Room (parallel)
            try {
                const token = localStorage.getItem('token') || JSON.parse(localStorage.getItem('user_info') || '{}').access_token || "";

                // Get Members
                invoke<string>('get_group_members', { groupId, token })
                    .then(async (resStr) => {
                        const res = JSON.parse(resStr);
                        if (res.data?.members) {
                            let mList = Array.isArray(res.data.members) ? res.data.members : [];

                            mList = mList.map((m: any) => ({
                                id: String(m.user_id || m.id || m.Member_Account),
                                name: m.user_name || m.name || m.student_name || "未知",
                                avatar: m.face_url || m.avatar || ""
                            }));

                            setMembers(mList);

                            if (res.data.group_info?.temp_room) {
                                const tr = res.data.group_info.temp_room;
                                setRoomInfo({
                                    room_id: tr.room_id,
                                    whip_url: tr.whip_url || tr.publish_url,
                                    whep_url: tr.whep_url || tr.play_url,
                                    stream_name: tr.stream_name || tr.room_id,
                                    group_id: groupId
                                });
                            }
                        }
                    })
                    .catch(err => log(`获取成员失败: ${err}`, 'error'));

                log(`正在检索活跃房间...`, 'info');
                invoke<string>('fetch_temp_room', { groupId: groupId })
                    .then(async (resStr) => {
                        const res = JSON.parse(resStr);
                        let tr = null;
                        if (res.code === 200 && res.data) {
                            if (Array.isArray(res.data.rooms) && res.data.rooms.length > 0) {
                                tr = res.data.rooms[0];
                            } else if (!Array.isArray(res.data) && res.data.room_id) {
                                tr = res.data;
                            } else if (Array.isArray(res.data) && res.data.length > 0) {
                                tr = res.data[0];
                            }
                        }

                        if (tr) {
                            log(`成功加入房间: ${tr.room_id}`, 'success');
                            setRoomInfo({
                                room_id: tr.room_id,
                                whip_url: tr.whip_url || tr.publish_url,
                                whep_url: tr.whep_url || tr.play_url,
                                stream_name: tr.stream_name || tr.room_id,
                                group_id: groupId
                            });
                        } else {
                            log('未找到活跃房间，正在为您创建...', 'warn');
                            try {
                                const toggleResStr = await invoke<string>('toggle_group_intercom', {
                                    groupId: groupId,
                                    enable: true
                                });
                                const toggleRes = JSON.parse(toggleResStr);

                                if (toggleRes.code === 200) {
                                    log('对讲房间已开启，正在同步...', 'info');
                                    setTimeout(async () => {
                                        const retryResStr = await invoke<string>('fetch_temp_room', { groupId: groupId });
                                        const retryRes = JSON.parse(retryResStr);
                                        let r = null;
                                        if (retryRes.data?.rooms && retryRes.data.rooms.length > 0) {
                                            r = retryRes.data.rooms[0];
                                        } else if (retryRes.data && retryRes.data[0]) {
                                            r = retryRes.data[0];
                                        }

                                        if (r) {
                                            log(`房间创建成功 ID: ${r.room_id}`, 'success');
                                            setRoomInfo({
                                                room_id: r.room_id,
                                                whip_url: r.whip_url,
                                                whep_url: r.whep_url,
                                                stream_name: r.stream_name || r.room_id,
                                                group_id: groupId
                                            });
                                        }
                                    }, 800);
                                } else {
                                    log(`开启对讲失败: ${toggleRes.msg}`, 'error');
                                }
                            } catch (e: any) {
                                log(`开启对讲请求错误: ${e}`, 'error');
                            }
                        }
                    })
                    .catch(err => log(`获取房间失败: ${err}`, 'error'));

            } catch (e) {
                log(`初始化数据失败: ${e}`, 'error');
            }

            connectWebSocket();
        };

        init();

        // Get current user ID for ID matching
        const uStr = localStorage.getItem('user_info');
        if (uStr) {
            try {
                const u = JSON.parse(uStr);
                const uid = u.teacher_unique_id || u.unique_id || u.user_id || u.class_code || u.id;
                if (uid) setMyId(String(uid));
            } catch (e) { console.error(e); }
        }

        return () => {
            if (wsRef.current) {
                wsRef.current.close();
                wsRef.current = null;
            }
            if (publishPcRef.current) {
                publishPcRef.current.close();
                publishPcRef.current = null;
            }
            if (subscribePcRef.current) {
                subscribePcRef.current.close();
                subscribePcRef.current = null;
            }
            if (publishStreamRef.current) {
                publishStreamRef.current.getTracks().forEach(t => t.stop());
                publishStreamRef.current = null;
            }
        };
    }, [groupId]);

    useEffect(() => {
        // We no longer auto-pull here. 
        // According to the server flow, we wait for 'voice_speaking' broadcast.
    }, [roomInfo, isWsConnected]);

    const connectWebSocket = () => {
        if (wsRef.current) return;

        let userId = localStorage.getItem('teacher_unique_id') || localStorage.getItem('userId') || localStorage.getItem('user_id');

        if (!userId) {
            const uStr = localStorage.getItem('user_info');
            if (uStr) {
                try {
                    const u = JSON.parse(uStr);
                    userId = u.teacher_unique_id || u.unique_id || u.user_id || u.class_code || u.id;
                } catch (e) { console.error(e); }
            }
        }

        if (!userId) {
            log('认证失败: 未获取到用户ID', 'error');
            return;
        }

        const url = `ws://47.100.126.194:5000/ws/${userId}`;
        log(`正在连接信令服务器...`, 'info');

        const ws = new WebSocket(url);
        wsRef.current = ws;

        ws.onopen = () => {
            if (ws !== wsRef.current) return;
            log('信令通道已建立', 'success');
            setIsWsConnected(true);
            requestMicPermission();
        };

        ws.onmessage = (event) => {
            if (ws !== wsRef.current) return;
            try {
                if (event.data === 'pong') return;
                const msg = JSON.parse(event.data);
                handleWsMessage(msg);
            } catch (e) { console.error(e); }
        };

        ws.onclose = () => {
            if (ws !== wsRef.current) return;
            log('信令通道已断开', 'error');
            setIsWsConnected(false);
        };

        ws.onerror = () => {
            if (ws !== wsRef.current) return;
            log('信令通道发生错误', 'error');
        };
    };

    const handleWsMessage = (msg: any) => {
        const type = msg.type || msg.action;
        console.log(`[Intercom-WS] Handling message type: ${type}`, msg);

        switch (msg.type) {
            case 'room_created':
            case '6':
                log(`房间已就绪: ${msg.room_id}`, 'success');
                console.log("[Intercom-Room] Full info received:", msg);
                const newInfo = {
                    room_id: msg.room_id,
                    whip_url: msg.whip_url || msg.publish_url,
                    whep_url: msg.whep_url || msg.play_url,
                    stream_name: msg.stream_name || msg.room_id,
                    group_id: msg.group_id || groupId || ""
                };
                setRoomInfo(newInfo);
                break;
            case 'srs_answer':
                console.log(`[Intercom-RTC] Received SRS Answer for ${msg.action}`, msg.sdp ? "SDP present" : "No SDP");
                handleSrsAnswer(msg);
                break;
            case 'voice_speaking':
                console.log("[Intercom-Status] Voice speaking update:", msg);
                if (msg.is_speaking) {
                    setSpeakingUser(msg.user_id);
                    // Server Flow Step 4: Other members automatically pull stream
                    const currentMyId = myIdRef.current;
                    const currentRoomInfo = roomInfoRef.current;
                    console.log(`[Intercom-Status] Check: speaker=${msg.user_id}, me=${currentMyId}, hasRoom=${!!currentRoomInfo}`);
                    if (String(msg.user_id) !== String(currentMyId) && currentRoomInfo) {
                        log(`正在接收来自 ${msg.user_name || msg.user_id} 的语音...`, 'info');
                        startPullStream(currentRoomInfo.whep_url || "", currentRoomInfo);
                    } else if (!currentRoomInfo) {
                        log('未缓存房间信息，暂时无法拉流', 'warn');
                    }
                } else {
                    if (speakingUser === String(msg.user_id)) {
                        setSpeakingUser(null);
                    }
                    // Server Flow Step 5: Stop pulling
                    const currentMyId = myIdRef.current;
                    if (String(msg.user_id) !== String(currentMyId)) {
                        if (subscribePcRef.current) {
                            log('停止接收语音', 'info');
                            console.log("[Intercom-RTC] Closing subscribe connection");
                            subscribePcRef.current.close();
                            subscribePcRef.current = null;
                        }
                    }
                }
                break;
            case 'temp_room_closed':
                log('房间已解散', 'warn');
                console.log("[Intercom-Room] Room closed by server");
                setRoomInfo(null);
                stopPublishing();
                break;
        }
    };

    const requestMicPermission = async () => {
        try {
            const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
            publishStreamRef.current = stream;
            log('麦克风权限已就绪', 'success');
            setIntercomEnabled(true);
        } catch (e: any) {
            log(`麦克风访问受阻: ${e.message}`, 'error');
        }
    };

    const startPullStream = async (_url: string, _rInfo: RoomInfo) => {
        if (!wsRef.current) {
            console.error("[Intercom-RTC] Cannot pull stream: WS not open");
            return;
        }
        try {
            console.log("[Intercom-RTC] Initializing subscribe peer connection...");
            if (subscribePcRef.current) {
                console.log("[Intercom-RTC] Closing existing subscribe connection");
                subscribePcRef.current.close();
            }
            const pc = new RTCPeerConnection({ iceServers: [{ urls: 'stun:stun.l.google.com:19302' }] });
            subscribePcRef.current = pc;

            pc.onconnectionstatechange = () => {
                console.log(`[Intercom-RTC-Play] Connection State: ${pc.connectionState}`);
                if (pc.connectionState === 'failed') {
                    log('拉流连接失败', 'error');
                }
            };

            pc.oniceconnectionstatechange = () => {
                console.log(`[Intercom-RTC-Play] ICE State: ${pc.iceConnectionState}`);
            };

            pc.ontrack = (ev) => {
                console.log("[Intercom-RTC-Play] Received remote track:", ev.track.kind);
                log('音频接收成功', 'success');
                const audio = document.createElement('audio');
                audio.srcObject = ev.streams[0];
                audio.autoplay = true;
                if (isMuted) audio.muted = true;
                (audio as any)._isIntercom = true;
            };

            pc.addTransceiver('audio', { direction: 'recvonly' });

            const offer = await pc.createOffer();
            console.log("[Intercom-RTC-Play] Created local offer");
            await pc.setLocalDescription(offer);

            const streamName = _rInfo.stream_name || _rInfo.room_id;
            console.log("[Intercom-RTC-Play] Sending srs_play request for:", streamName);
            wsRef.current.send(JSON.stringify({
                type: "srs_play",
                sdp: pc.localDescription?.sdp,
                stream_name: streamName,
                group_id: _rInfo.group_id
            }));
            log('已发送拉流请求', 'info');
        } catch (err: any) {
            console.error("[Intercom-RTC-Play] Setup error:", err);
            log(`音频接收失败: ${err.message}`, 'error');
        }
    };

    const startPublishing = async () => {
        if (!roomInfo || !wsRef.current) {
            console.error("[Intercom-RTC-Publish] Cannot publish: Room or WS missing");
            log('推流失败: 房间未就绪', 'error');
            return;
        }
        if (isTalkingRef.current) return;
        isTalkingRef.current = true;
        try {
            console.log("[Intercom-RTC-Publish] Initializing publish peer connection...");
            log('正在开启对讲...', 'info');
            setIsPublishing(true);
            if (publishPcRef.current) publishPcRef.current.close();
            const pc = new RTCPeerConnection({ iceServers: [{ urls: 'stun:stun.l.google.com:19302' }] });
            publishPcRef.current = pc;

            pc.onconnectionstatechange = () => {
                console.log(`[Intercom-RTC-Publish] Connection State: ${pc.connectionState}`);
                if (pc.connectionState === 'failed') {
                    log('推流连接失败', 'error');
                }
            };

            pc.oniceconnectionstatechange = () => {
                console.log(`[Intercom-RTC-Publish] ICE State: ${pc.iceConnectionState}`);
            };

            let stream = publishStreamRef.current;
            if (!stream || stream.getTracks().some(t => t.readyState === 'ended')) {
                console.log("[Intercom-RTC-Publish] Requesting user media (audio)...");
                stream = await navigator.mediaDevices.getUserMedia({ audio: true });
                publishStreamRef.current = stream;
            }

            if (!isTalkingRef.current) {
                console.warn("[Intercom-RTC-Publish] Cancelled by user before media ready");
                pc.close();
                publishPcRef.current = null;
                setIsPublishing(false);
                return;
            }

            console.log("[Intercom-RTC-Publish] Adding local audio track to connection");
            stream.getTracks().forEach(track => pc.addTrack(track, stream));

            const offer = await pc.createOffer();
            console.log("[Intercom-RTC-Publish] Created local offer");
            await pc.setLocalDescription(offer);

            if (!isTalkingRef.current) {
                console.warn("[Intercom-RTC-Publish] Cancelled by user before offer sent");
                pc.close();
                publishPcRef.current = null;
                setIsPublishing(false);
                return;
            }

            const streamName = roomInfo.stream_name || roomInfo.room_id;
            console.log("[Intercom-RTC-Publish] Sending srs_publish request for:", streamName);
            
            // 1. 发送推流 SDP
            wsRef.current.send(JSON.stringify({
                type: "srs_publish",
                sdp: pc.localDescription?.sdp,
                stream_name: streamName,
                group_id: roomInfo.group_id
            }));

            // 2. 主动上报正在说话状态，触发全员广播
            wsRef.current.send(JSON.stringify({
                type: "voice_speaking",
                group_id: roomInfo.group_id,
                is_speaking: true
            }));

        } catch (e: any) {
            console.error("[Intercom-RTC-Publish] Setup error:", e);
            log(`推流失败: ${e.message}`, 'error');
            setIsPublishing(false);
            isTalkingRef.current = false;
        }
    };

    const stopPublishing = () => {
        if (!isTalkingRef.current) return;
        log('对讲已停止', 'info');
        setIsPublishing(false);
        isTalkingRef.current = false;
        if (publishPcRef.current) {
            publishPcRef.current.close();
            publishPcRef.current = null;
        }

        // Server Flow Step 5: Send state update to server
        if (wsRef.current && roomInfo) {
            wsRef.current.send(JSON.stringify({
                type: "voice_speaking",
                group_id: roomInfo.group_id,
                is_speaking: false
            }));
        }
    };

    const handleSrsAnswer = async (msg: any) => {
        try {
            if (msg.action === 'publish' && publishPcRef.current) {
                await publishPcRef.current.setRemoteDescription({ type: 'answer', sdp: msg.sdp });
                log('对讲通道已建立', 'success');
            } else if (msg.action === 'play' && subscribePcRef.current) {
                await subscribePcRef.current.setRemoteDescription({ type: 'answer', sdp: msg.sdp });
                log('音频接收已就绪', 'success');
            }
        } catch (err: any) {
            log(`信令握手失败: ${err.message}`, 'error');
        }
    };

    const toggleMute = () => {
        const newMuted = !isMuted;
        setIsMuted(newMuted);
        const audios = document.querySelectorAll('audio');
        audios.forEach(a => {
            if ((a as any)._isIntercom) a.muted = newMuted;
        });
        log(newMuted ? '已开启静音' : '已关闭静音', 'warn');
    };

    return (
        <div className="h-screen w-screen bg-slate-50 text-gray-800 flex flex-col items-center overflow-hidden font-sans">
            {/* Custom Styles for Animations */}
            <style>{`
                @keyframes pulse-ring {
                    0% { transform: scale(.8); opacity: 0.5; }
                    50% { opacity: 1; }
                    100% { transform: scale(1.2); opacity: 0; }
                }
                @keyframes wave {
                    0%, 100% { transform: scaleY(0.4); }
                    50% { transform: scaleY(1); }
                }
                .scrollbar-hide::-webkit-scrollbar { display: none; }
                .light-panel {
                    background: #ffffff;
                    border: 1px solid #f1f5f9;
                    box-shadow: 0 1px 3px 0 rgba(0, 0, 0, 0.05);
                }
            `}</style>

            {/* Header */}
            <div className="w-full h-14 px-6 flex justify-between items-center bg-white border-b border-gray-100 shadow-sm z-10" data-tauri-drag-region>
                <div className="flex items-center gap-3 select-none">
                    <div className="w-8 h-8 bg-blue-600 rounded-lg flex items-center justify-center shadow-md shadow-blue-600/10">
                        <Radio size={18} className="text-white" />
                    </div>
                    <div>
                        <h1 className="text-sm font-bold text-gray-800">语音对讲机</h1>
                        <div className="flex items-center gap-1.5">
                            <span className={`w-1.5 h-1.5 rounded-full ${roomInfo ? 'bg-green-500 animate-pulse' : 'bg-amber-500'}`} />
                            <span className="text-[10px] text-gray-500 uppercase tracking-widest font-semibold">
                                {roomInfo ? '已连接' : '正在连接...'}
                            </span>
                        </div>
                    </div>
                </div>
                
                <div className="flex items-center gap-2">
                    <button 
                        onClick={toggleMute}
                        className={`p-2 rounded-lg transition-all ${isMuted ? 'bg-red-50 text-red-500' : 'bg-gray-100 text-gray-500 hover:bg-gray-200 hover:text-gray-700'}`}
                        title={isMuted ? "取消静音" : "静音"}
                    >
                        {isMuted ? <VolumeX size={18} /> : <Volume2 size={18} />}
                    </button>
                    <button 
                        onClick={() => window.close()} 
                        className="p-2 rounded-lg bg-gray-100 text-gray-500 hover:bg-rose-500 hover:text-white transition-all shadow-sm"
                    >
                        <X size={18} />
                    </button>
                </div>
            </div>

            {/* Main Content Area */}
            <div className="flex-1 w-full max-w-5xl flex flex-col p-6 gap-6 overflow-hidden">
                
                {/* Participants Section */}
                <div className="light-panel rounded-2xl p-5 flex flex-col gap-4">
                    <div className="flex items-center justify-between px-1">
                        <div className="flex items-center gap-2 text-gray-500">
                            <Users size={14} className="text-blue-500" />
                            <span className="text-xs font-bold uppercase tracking-wider">当前在线 ({members.length})</span>
                        </div>
                    </div>
                    
                    <div className="flex gap-5 overflow-x-auto pb-2 scrollbar-hide">
                        {members.map(m => (
                            <div key={m.id} className="flex flex-col items-center gap-3 group">
                                <div className="relative">
                                    {/* Speaking Ring */}
                                    {speakingUser === String(m.id) && (
                                        <div className="absolute inset-0 -m-1.5 rounded-2xl border-2 border-green-500/50 animate-[pulse-ring_1.5s_infinite]" />
                                    )}
                                    
                                    <div className={`
                                        w-14 h-14 rounded-2xl flex items-center justify-center text-lg font-bold transition-all duration-300
                                        ${speakingUser === String(m.id) 
                                            ? 'ring-2 ring-green-500 scale-105 shadow-lg shadow-green-500/10' 
                                            : 'bg-slate-100 ring-1 ring-gray-100 group-hover:ring-gray-200 group-hover:scale-105'}
                                    `}>
                                        {m.avatar ? (
                                            <img src={m.avatar} className="w-full h-full object-cover rounded-2xl" alt={m.name} />
                                        ) : (
                                            <div className="w-full h-full rounded-2xl bg-blue-50 flex items-center justify-center text-blue-600">
                                                {m.name[0]}
                                            </div>
                                        )}
                                        
                                        {/* Status Dot */}
                                        <div className={`absolute -bottom-0.5 -right-0.5 w-3.5 h-3.5 rounded-full border-2 border-white ${speakingUser === String(m.id) ? 'bg-green-500' : 'bg-gray-300'}`} />
                                    </div>
                                </div>
                                <span className={`text-[11px] font-bold transition-colors ${speakingUser === String(m.id) ? 'text-green-600' : 'text-gray-600 group-hover:text-gray-900'}`}>
                                    {m.name}
                                </span>
                            </div>
                        ))}
                        {members.length === 0 && (
                            <div className="w-full py-4 flex items-center justify-center text-gray-400 text-xs italic">
                                正在同步成员列表...
                            </div>
                        )}
                    </div>
                </div>

                {/* Status Log Section */}
                <div className="flex-[2] light-panel rounded-2xl flex flex-col overflow-hidden min-h-[200px]">
                    <div className="px-4 py-2.5 bg-slate-50 flex items-center justify-between border-b border-gray-100">
                        <div className="flex items-center gap-2">
                            <Terminal size={12} className="text-blue-500" />
                            <span className="text-[10px] font-bold text-gray-500 uppercase tracking-widest">系统日志</span>
                        </div>
                        <span className="text-[9px] text-gray-400 font-mono">CONSOLE ACTIVE</span>
                    </div>
                    <div 
                        ref={statusLogRef} 
                        className="flex-1 overflow-y-auto p-4 space-y-1.5 font-mono text-[11px] leading-relaxed bg-white scrollbar-thin scrollbar-thumb-gray-200"
                    >
                        {statusLogs.map((l, i) => (
                            <div key={i} className="flex gap-3 animate-in fade-in slide-in-from-left-1 duration-200">
                                <span className="text-gray-400 shrink-0 select-none">[{l.msg.split(' ')[0]}]</span>
                                <span className={`
                                    font-medium break-all
                                    ${l.type === 'error' ? 'text-rose-600' : 
                                      l.type === 'success' ? 'text-emerald-600' : 
                                      l.type === 'warn' ? 'text-amber-600' : 
                                      'text-blue-600'}
                                `}>
                                    {l.msg.split(' ').slice(1).join(' ')}
                                </span>
                            </div>
                        ))}
                        {statusLogs.length === 0 && (
                            <div className="text-gray-300 italic py-2">等待系统事件...</div>
                        )}
                    </div>
                </div>

                {/* Footer Controls */}
                <div className="h-36 flex flex-col items-center justify-center gap-4 relative">
                    {/* Background Glow */}
                    <div className={`absolute w-48 h-48 rounded-full blur-[60px] transition-all duration-700 opacity-10 ${isPublishing ? 'bg-red-500' : 'bg-blue-500'}`} />
                    
                    {/* Wave Animation when publishing */}
                    {isPublishing && (
                        <div className="flex items-center gap-1.5 h-6 mb-2">
                            {[1, 2, 3, 4, 5, 6, 7, 8].map(i => (
                                <div 
                                    key={i} 
                                    className="w-1.5 bg-red-500 rounded-full animate-[wave_1s_ease-in-out_infinite]" 
                                    style={{ animationDelay: `${i * 0.1}s`, height: `${Math.random() * 80 + 20}%` }}
                                />
                            ))}
                        </div>
                    )}

                    <div className="relative">
                        {/* Pulse Ring for active publishing */}
                        {isPublishing && (
                            <div className="absolute inset-0 -m-4 rounded-full border-4 border-red-500/20 animate-[pulse-ring_2s_infinite]" />
                        )}
                        
                        <button
                            disabled={!intercomEnabled || !roomInfo}
                            onMouseDown={startPublishing}
                            onMouseUp={stopPublishing}
                            onMouseLeave={stopPublishing}
                            onTouchStart={startPublishing}
                            onTouchEnd={stopPublishing}
                            className={`
                                w-24 h-24 rounded-full flex items-center justify-center shadow-xl transition-all duration-300 select-none relative z-10
                                ${!intercomEnabled || !roomInfo 
                                    ? 'bg-gray-200 text-gray-400 cursor-not-allowed border border-gray-100' 
                                    : isPublishing 
                                        ? 'bg-rose-500 text-white scale-110 shadow-rose-500/30' 
                                        : 'bg-blue-600 text-white hover:bg-blue-500 hover:scale-105 active:scale-95 shadow-blue-600/20'}
                            `}
                        >
                            {isPublishing ? <MicOff size={32} strokeWidth={2.5} /> : <Mic size={32} strokeWidth={2.5} />}
                        </button>
                    </div>
                    
                    <div className="flex flex-col items-center gap-1">
                        <p className={`text-xs font-bold tracking-wide transition-colors ${isPublishing ? 'text-rose-600' : 'text-gray-700'}`}>
                            {isPublishing ? "正在通话中..." : "按住按钮开始对讲"}
                        </p>
                        <p className="text-[10px] text-gray-400 uppercase tracking-[0.2em] font-bold">
                            {intercomEnabled ? "PUSH TO TALK" : "Initializing..."}
                        </p>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default IntercomWindow;
