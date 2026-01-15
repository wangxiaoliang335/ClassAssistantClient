import { invoke } from "@tauri-apps/api/core";

// Singleton WebSocket instance
let socket: WebSocket | null = null;
let heartbeatInterval: any = null;
let reconnectTimeout: any = null;
let currentUserId: string = "";

// Cache for notification data
let latestNotifications: any[] = [];

const WS_URL_BASE = "ws://47.100.126.194:5000/ws/";

export const getLatestNotifications = () => latestNotifications;

export const connectWS = (userId: string) => {
    if (socket && socket.readyState === WebSocket.OPEN) {
        console.log("[WS] Already connected");
        return;
    }

    currentUserId = userId;
    const url = WS_URL_BASE + userId;
    console.log("[WS] Connecting to", url);

    socket = new WebSocket(url);

    socket.onopen = () => {
        console.log("[WS] Connected");
        startHeartbeat();
    };

    socket.onmessage = (event) => {
        const msg = event.data;
        if (msg === "pong") {
            // console.log("[WS] Pong received");
            return;
        }
        console.log("[WS] Message received:", msg);

        try {
            const parsed = JSON.parse(msg);
            if (parsed.type === "unread_notifications") {
                latestNotifications = parsed.data || [];
                console.log("[WS] Cached unread notifications:", latestNotifications.length);
            }
        } catch (e) {
            // ignore parse error for non-json
        }

        // Dispatch event for other components to listen if needed
        window.dispatchEvent(new CustomEvent("ws-message", { detail: msg }));
    };

    socket.onclose = () => {
        console.log("[WS] Disconnected");
        stopHeartbeat();
        // Auto reconnect after 3s
        if (reconnectTimeout) clearTimeout(reconnectTimeout);
        reconnectTimeout = setTimeout(() => {
            console.log("[WS] Reconnecting...");
            connectWS(currentUserId);
        }, 3000);
    };

    socket.onerror = (error) => {
        console.error("[WS] Error:", error);
    };
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

