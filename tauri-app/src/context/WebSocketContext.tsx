import React, { createContext, useContext, useEffect, useState } from 'react';
import { connectWS, sendMessageWS } from '../utils/websocket';

interface WebSocketContextType {
    sendMessage: (msg: string) => void;
    isConnected: boolean;
    lastMessage: string | null;
}

const WebSocketContext = createContext<WebSocketContextType | null>(null);

export const WebSocketProvider = ({ children }: { children: React.ReactNode }) => {
    const [isConnected, setIsConnected] = useState(false);
    const [lastMessage, setLastMessage] = useState<string | null>(null);

    useEffect(() => {
        let reconnectTimer: any = null;

        const checkAndConnect = () => {
            const storedUser = localStorage.getItem('user_info');
            let teacherId = null;
            if (storedUser) {
                try {
                    const u = JSON.parse(storedUser);
                    teacherId = u.teacher_unique_id || u.unique_id || u.id || u.class_code || u.class_id;
                } catch (e) { }
            }

            if (teacherId) {
                connectWS(teacherId);
            } else {
                reconnectTimer = setTimeout(checkAndConnect, 2000);
            }
        };

        const handleStatus = (e: any) => {
            setIsConnected(e.detail.connected);
        };

        const handleMessage = (e: any) => {
            setLastMessage(e.detail);
        };

        window.addEventListener('ws-status', handleStatus as EventListener);
        window.addEventListener('ws-message', handleMessage as EventListener);

        checkAndConnect();

        return () => {
            window.removeEventListener('ws-status', handleStatus as EventListener);
            window.removeEventListener('ws-message', handleMessage as EventListener);
            if (reconnectTimer) clearTimeout(reconnectTimer);
        };
    }, []);

    const sendMessage = (msg: string) => {
        sendMessageWS(msg);
    };

    return (
        <WebSocketContext.Provider value={{ sendMessage, isConnected, lastMessage }}>
            {children}
        </WebSocketContext.Provider>
    );
};

export const useWebSocket = () => {
    const context = useContext(WebSocketContext);
    if (!context) {
        throw new Error("useWebSocket must be used within a WebSocketProvider");
    }
    return context;
};

// Internal reference for timeout cleanup
// let reconnectTimeout: any = null;
