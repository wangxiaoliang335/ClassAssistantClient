// import { invoke } from "@tauri-apps/api/core"; // Unused

// Singleton WebSocket instance
let socket: WebSocket | null = null;
let heartbeatInterval: any = null;
let reconnectTimeout: any = null;
let currentUserId: string = "";
let isConnecting: boolean = false;

// Cache for notification data
let latestNotifications: any[] = [];

const WS_URL_BASE = "ws://47.100.126.194:5000/ws/";

export const getLatestNotifications = () => latestNotifications;

export const connectWS = (userId: string) => {
    if (!userId) return;
    
    // Prevent duplicate connection attempts
    if (isConnecting) return;
    
    if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
        // console.log("%c[WS-Debug] Socket already OPEN or CONNECTING, skipping.", "color: #9e9e9e;");
        return;
    }

    console.log(`%c[WS-Debug] connectWS called with userId: ${userId}`, "background: #222; color: #ffeb3b; padding: 2px 5px;");
    
    isConnecting = true;
    currentUserId = userId;
    const url = WS_URL_BASE + userId;
    console.log(`%c[WS-Debug] Attempting connection to: ${url}`, "color: #2196F3; font-weight: bold;");

    try {
        socket = new WebSocket(url);

        socket.onopen = () => {
            isConnecting = false;
            console.log("%c[WS-Debug] CONNECTION ESTABLISHED!", "background: #4CAF50; color: white; padding: 2px 5px; font-weight: bold;");
            startHeartbeat();
            window.dispatchEvent(new CustomEvent("ws-status", { detail: { connected: true } }));
        };

        socket.onmessage = (event) => {
            const msg = event.data;
            if (msg === "pong") return;
            
            try {
                const parsed = JSON.parse(msg);
                
                // Detailed logging for Intercom-related messages
                const intercomTypes = ["6", "room_created", "srs_answer", "voice_speaking", "temp_room_closed"];
                if (intercomTypes.includes(parsed.type) || (parsed.action && ["publish", "play"].includes(parsed.action))) {
                    console.log(`%c[WS-Intercom] RECV: ${parsed.type || parsed.action}`, "background: #2196F3; color: white; padding: 1px 3px;", parsed);
                }

                if (parsed.type === "unread_notifications") {
                    latestNotifications = parsed.data || [];
                }
            } catch (e) { }

            // Global event for all components
            window.dispatchEvent(new CustomEvent("ws-message", { detail: msg }));
        };

        socket.onclose = (event) => {
            isConnecting = false;
            console.log(`%c[WS-Debug] DISCONNECTED. Code: ${event.code}`, "background: #f44336; color: white; padding: 2px 5px;");
            stopHeartbeat();
            window.dispatchEvent(new CustomEvent("ws-status", { detail: { connected: false } }));
            
            if (reconnectTimeout) clearTimeout(reconnectTimeout);
            reconnectTimeout = setTimeout(() => {
                connectWS(currentUserId);
            }, 3000);
        };

        socket.onerror = (error) => {
            isConnecting = false;
            console.error("%c[WS-Error] WebSocket Error detected:", "background: red; color: white;", error);
        };
    } catch (err) {
        isConnecting = false;
        console.error("Failed to create WebSocket:", err);
    }
};

export const disconnectWS = () => {
    if (socket) {
        socket.close();
        socket = null;
    }
    stopHeartbeat();
    if (reconnectTimeout) clearTimeout(reconnectTimeout);
    latestNotifications = []; // Clear cache on explicit disconnect? Or keep it?
};

const startHeartbeat = () => {
    stopHeartbeat();
    heartbeatInterval = setInterval(() => {
        if (socket && socket.readyState === WebSocket.OPEN) {
            socket.send("ping");
            // console.log("[WS] Ping sent");
        }
    }, 5000); // Send ping every 5 seconds
};

const stopHeartbeat = () => {
    if (heartbeatInterval) {
        clearInterval(heartbeatInterval);
        heartbeatInterval = null;
    }
};

export const sendMessageWS = (msg: string) => {
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(msg);
    } else {
        console.warn("[WS] Cannot send message, socket not open");
    }
};

