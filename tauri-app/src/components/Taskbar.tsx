import { useState } from 'react';
import { clsx, type ClassValue } from 'clsx';
import { twMerge } from 'tailwind-merge';
import { Folder, Calendar, Users } from 'lucide-react';

function cn(...inputs: ClassValue[]) {
    return twMerge(clsx(inputs));
}

// Define Dock Items types - reusing same types for compatibility
export type TaskbarItemType = 'folder' | 'schedule' | 'user';

interface TaskbarProps {
    activeItem: TaskbarItemType | null;
    onItemClick: (item: TaskbarItemType) => void;
}

const Taskbar = ({ activeItem, onItemClick }: TaskbarProps) => {
    const [hoveredItem, setHoveredItem] = useState<TaskbarItemType | null>(null);

    const taskbarItems = [
        { id: 'folder', label: '资源云盘', icon: Folder, color: 'text-blue-600' },
        { id: 'schedule', label: '课程表', icon: Calendar, color: 'text-orange-600' },
        { id: 'user', label: '班级信息', icon: Users, color: 'text-purple-600' },
    ] as const;

    return (
        <div className="fixed bottom-0 left-0 right-0 z-50 flex justify-center pb-2 pointer-events-none">
            {/* Windows 11 Taskbar Container */}
            <div className="pointer-events-auto h-12 px-2 bg-[#f3f3f3]/85 backdrop-blur-xl border border-white/40 shadow-lg rounded-lg flex items-center gap-1 transition-all duration-300">

                {/* Start Menu Placeholder (Optional, for authentic feel) */}
                <div className="w-10 h-10 flex items-center justify-center rounded hover:bg-white/50 transition-colors mx-1 cursor-pointer group">
                    <svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg" className="group-hover:opacity-80 transition-opacity">
                        <path d="M4 4H11V11H4V4Z" fill="#0078D7" />
                        <path d="M4 13H11V20H4V13Z" fill="#0078D7" />
                        <path d="M13 4H20V11H13V4Z" fill="#0078D7" />
                        <path d="M13 13H20V20H13V13Z" fill="#0078D7" />
                    </svg>
                </div>

                <div className="w-[1px] h-6 bg-gray-300/50 mx-1"></div>

                {taskbarItems.map((item) => {
                    const Icon = item.icon;
                    const isActive = activeItem === item.id;
                    const isHovered = hoveredItem === item.id;

                    return (
                        <button
                            key={item.id}
                            onClick={() => onItemClick(item.id as TaskbarItemType)}
                            onMouseEnter={() => setHoveredItem(item.id as TaskbarItemType)}
                            onMouseLeave={() => setHoveredItem(null)}
                            className={cn(
                                "group relative w-10 h-10 flex items-center justify-center rounded hover:bg-white/60 active:bg-white/40 transition-all duration-200",
                                isActive && "bg-white/60"
                            )}
                            title={item.label}
                        >
                            {/* Active Indicator (Pill at bottom) */}
                            <div className={cn(
                                "absolute bottom-1 w-3 h-1 rounded-full bg-blue-500 transition-all duration-300 transform scale-x-0 opacity-0",
                                isActive && "scale-x-100 opacity-100",
                                isHovered && !isActive && "scale-x-50 opacity-50 bg-gray-400"
                            )}></div>

                            {/* Icon */}
                            <Icon
                                size={22}
                                className={cn(
                                    "text-gray-600 transition-transform duration-200",
                                    isActive && item.color, // Colorize when active
                                    "active:scale-95"
                                )}
                                strokeWidth={2}
                            />
                        </button>
                    );
                })}
            </div>
        </div>
    );
};

export default Taskbar;
