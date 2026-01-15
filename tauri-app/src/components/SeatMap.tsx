import { useState, useEffect, useCallback } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { User, Loader2, Save, Users, Trash2 } from 'lucide-react';
import { useParams } from 'react-router-dom';

export interface SeatInfo {
    row: number; // 0-indexed
    col: number; // 0-indexed
    student_name?: string;
    student_id?: string;
    name?: string;
}

export interface Student {
    id: string;
    name: string;
    score: number;
    attendance: 'present' | 'late' | 'absent';
    group?: string;
}

interface SeatProps {
    classId: string | undefined;
    onViewChange?: (view: 'list' | 'grid') => void;
    key?: number;
    colorMap?: Record<string, string>;
    isHeatmapMode?: boolean;
}

const SeatMap: React.FC<SeatProps> = ({ classId, onViewChange, colorMap, isHeatmapMode = false }) => {
    const { groupclassId } = useParams();
    // State
    const [seatData, setSeatData] = useState<SeatInfo[]>([]);
    const [loading, setLoading] = useState(false);
    const [mode, setMode] = useState<'view' | 'edit'>('view');
    // const [heatmapMode, setHeatmapMode] = useState(false); // Removed unused state
    const [selectedSeat, setSelectedSeat] = useState<SeatInfo | null>(null);
    const [students, setStudents] = useState<Student[]>([]);
    const [isAutoArranging, setIsAutoArranging] = useState(false);
    const [classMembers, setClassMembers] = useState<any[]>([]); // Store full class list

    // Constants
    const ROWS = 8;
    const COLS = 11;

    // Derive classId and groupId
    // groupclassId is typically the Group ID (e.g. 00001100201)
    // class_id is the prefix (e.g. 000011002)
    const groupId = groupclassId;
    // The classId prop is now passed directly, so we don't need to derive it from groupclassId here.
    // However, the original code derived `classId` from `groupclassId` if it ended with '01'.
    // Let's assume the `classId` prop is the correct one to use for fetching seat map.
    // If `classId` prop is undefined, we can fall back to deriving from `groupclassId` for consistency with original logic.
    const effectiveClassId = classId || (groupclassId?.endsWith('01') ? groupclassId.slice(0, -2) : groupclassId);


    // Fetch Class Members for Auto-Arrange
    const fetchClassMembers = async () => {
        if (!groupId) return;
        try {
            // get_group_members uses the Group ID (with 01)
            const res = await invoke('get_group_members', { groupId: groupId, token: localStorage.getItem('token') || '' }) as string;
            const json = JSON.parse(res);
            // Verify structure. Usually { code: 200, data: [...] } or just [...]
            console.log("SeatMap: Class Members:", json);
            if (json.code === 200 && Array.isArray(json.data)) {
                setClassMembers(json.data);
            } else if (Array.isArray(json.MemberList)) {
                // TIM style or other API style
                setClassMembers(json.MemberList);
            }
        } catch (e) {
            console.error("Failed to fetch class members", e);
        }
    };

    // Fetch Seat Map
    const fetchSeatMap = useCallback(async () => {
        if (!classId) {
            console.warn("SeatMap: Missing classId");
            return;
        }
        console.log("SeatMap: Fetching data for class:", classId);
        setLoading(true);
        try {
            const res = await invoke('fetch_seat_map', { classId: classId }) as string;
            console.log("SeatMap: API Response:", res);
            const json = JSON.parse(res);
            if (json.code === 200 && json.data && json.data.seats) {
                // Backend returns 1-indexed rows/cols mostly? 
                // Qt logic: seatObj["row"] = row + 1;
                // So we need to subtract 1.
                const seats = json.data.seats.map((s: any) => ({
                    ...s,
                    row: s.row - 1, // Qt 1-based to 0-based
                    col: s.col - 1
                }));
                setSeatData(seats);
                console.log("SeatMap: Parsed seats:", seats);
            } else {
                console.log("SeatMap: No existing map found. Ready to create.");
                // If no map, maybe we should fetch members automatically to be ready?
                fetchClassMembers();
            }
        } catch (e) {
            console.error("Failed to fetch seat map", e);
        } finally {
            setLoading(false);
        }
    }, [classId]);

    // Auto Arrange Logic
    const handleAutoArrange = async () => {
        if (classMembers.length === 0) {
            await fetchClassMembers();
            // Verify if we got members now
            // Due to async state update, we might need to rely on the just-fetched data or re-run this function?
            // For simplicity, let's just trigger fetch and alert user to try again if empty, 
            // or we use the data directly if we refactored fetchClassMembers to return data.
            // Let's refactor slightly above to be safer or just return data from fetch.
            alert("正在获取班级成员信息，请稍后再试一次 '自动排座'");
            return;
        }

        const newSeats: SeatInfo[] = [];
        let memberIndex = 0;

        for (let r = 0; r < ROWS; r++) {
            for (let c = 0; c < COLS; c++) {
                if (memberIndex >= classMembers.length) break;

                // Skip non-seat slots
                if (!isSeatSlot(r, c)) continue;
                // Skip Podium span in row 0
                if (r === 0 && [4, 5, 6].includes(c)) continue;

                const member = classMembers[memberIndex];
                newSeats.push({
                    row: r,
                    col: c,
                    student_name: member.NameCard || member.NickName || member.Member_Account, // Adjust based on actual member object
                    student_id: member.Member_Account,
                    name: member.NameCard || member.NickName // generic name field
                });
                memberIndex++;
            }
        }
        setSeatData(newSeats);
    };

    // Save Logic
    const handleSave = async () => {
        if (!classId) return;
        try {
            // Qt logic expects 1-based indexing for row/col
            const seatsToSave = seatData.map(s => ({
                ...s,
                row: s.row + 1,
                col: s.col + 1
            }));

            const res = await invoke('save_seat_map', { classId: classId, seats: seatsToSave }) as string;
            const json = JSON.parse(res);
            if (json.code === 200) {
                alert("座位表保存成功！");
            } else {
                alert("保存失败：" + (json.message || "未知错误"));
            }
        } catch (e) {
            console.error("Save failed", e);
            alert("保存失败，请检查网络或重试。");
        }
    };

    const handleClear = () => {
        if (confirm("确定要清空座位表吗？")) {
            setSeatData([]);
        }
    };

    useEffect(() => {
        console.log("SeatMap: Component Mounted");
        fetchSeatMap();
        fetchClassMembers(); // Pre-fetch members so auto-arrange is ready
    }, [fetchSeatMap]);

    // Helper to check if a cell is a valid seat slot based on Qt logic
    const isSeatSlot = (row: number, col: number) => {
        // Row 0 Logic
        if (row === 0) {
            // Seats at: 0,1, 3, 7, 9,10
            return [0, 1, 3, 7, 9, 10].includes(col);
        }
        // Rows 1-7 Logic: Seats at 0,1, 3,4, 6,7, 9,10
        // Aisles at 2, 5, 8
        const aisles = [2, 5, 8];
        return !aisles.includes(col);
    };

    const getSeatContent = (row: number, col: number) => {
        const seat = seatData.find(s => s.row === row && s.col === col);
        if (!seat) return null;

        // Filter out imported podium placeholders (we already have a hardcoded podium)
        const rawName = seat.student_name || seat.name || '';
        const cleanName = rawName.replace(/\s+/g, '');

        if (cleanName === '讲台' || cleanName.includes('讲台')) {
            return null;
        }

        return seat;
    };

    if (loading) {
        return (
            <div className="w-full h-full flex items-center justify-center p-4 bg-gray-50/50 rounded-2xl border border-gray-200 shadow-inner">
                <Loader2 className="animate-spin text-blue-500" size={32} />
            </div>
        );
    }

    return (
        <div className="w-full h-full p-4 bg-gray-50/50 rounded-2xl border border-gray-200 shadow-inner flex flex-col overflow-hidden">

            {/* Toolbar - Static Layout to avoid blocking seats */}
            <div className="flex justify-end gap-2 mb-2 shrink-0">
                <button onClick={handleAutoArrange} className="bg-white p-2 rounded-lg shadow-sm border border-gray-200 text-gray-600 hover:text-blue-500 hover:border-blue-300 transition-all flex items-center gap-1 text-xs font-medium" title="自动排座">
                    <Users size={16} /> 自动排座
                </button>
                <button onClick={handleClear} className="bg-white p-2 rounded-lg shadow-sm border border-gray-200 text-gray-600 hover:text-red-500 hover:border-red-300 transition-all flex items-center gap-1 text-xs font-medium" title="清空">
                    <Trash2 size={16} /> 清空
                </button>
                <button onClick={handleSave} className="bg-blue-500 p-2 rounded-lg shadow-md text-white hover:bg-blue-600 transition-all flex items-center gap-1 text-xs font-medium" title="保存">
                    <Save size={16} /> 保存
                </button>
            </div>

            {/* Grid Container */}
            <div className="flex-1 overflow-auto relative">
                <div className="min-h-full flex items-center justify-center p-4 relative">
                    {/* Heatmap Blur Layer (inside scrollable content) */}
                    {isHeatmapMode && colorMap && Object.keys(colorMap).length > 0 && (
                        <div className="absolute inset-0 z-0 pointer-events-none overflow-hidden">
                            <div className="w-full h-full" style={{ filter: 'blur(30px) saturate(1.4)', transform: 'scale(1.1)' }}>
                                <div
                                    className="grid w-full h-full p-4"
                                    style={{
                                        gridTemplateColumns: `repeat(${COLS}, minmax(60px, 1fr))`,
                                        gridTemplateRows: `repeat(${ROWS}, minmax(40px, 1fr))`,
                                        maxWidth: '1200px',
                                        margin: '0 auto',
                                    }}
                                >
                                    {Array.from({ length: ROWS }).map((_, rIndex) => (
                                        Array.from({ length: COLS }).map((_, cIndex) => {
                                            const isSeat = isSeatSlot(rIndex, cIndex);
                                            const data = getSeatContent(rIndex, cIndex);
                                            const color = data && colorMap && (colorMap[data.student_id!] || colorMap[data.student_name!]);

                                            // Skip podium cells
                                            if (rIndex === 0 && [4, 5, 6].includes(cIndex)) {
                                                return <div key={`blur-${rIndex}-${cIndex}`} />;
                                            }

                                            if (!isSeat || !color) {
                                                return <div key={`blur-${rIndex}-${cIndex}`} />;
                                            }

                                            return (
                                                <div
                                                    key={`blur-${rIndex}-${cIndex}`}
                                                    className="rounded-lg"
                                                    style={{
                                                        backgroundColor: color,
                                                        transform: 'scale(1.5)',
                                                    }}
                                                />
                                            );
                                        })
                                    ))}
                                </div>
                            </div>
                        </div>
                    )}

                    <div
                        className="grid gap-2 relative z-10"
                        style={{
                            gridTemplateColumns: `repeat(${COLS}, minmax(60px, 1fr))`,
                            gridTemplateRows: `repeat(${ROWS}, minmax(40px, 1fr))`,
                            width: '100%',
                            maxWidth: '1200px', // constrain max width
                            aspectRatio: '11/8' // try to keep ratio
                        }}
                    >
                        {Array.from({ length: ROWS }).map((_, rIndex) => (
                            Array.from({ length: COLS }).map((_, cIndex) => {
                                // Podium Logic (Row 0, Col 4 spans 3)
                                if (rIndex === 0 && cIndex === 4) {
                                    return (
                                        <div
                                            key={`podium-${rIndex}-${cIndex}`}
                                            className="bg-[#8B4513] text-white rounded-lg shadow-md flex items-center justify-center font-bold text-sm select-none"
                                            style={{ gridColumn: 'span 3' }}
                                        >
                                            讲 台
                                        </div>
                                    );
                                }
                                // Skip the columns spanned by Podium
                                if (rIndex === 0 && [5, 6].includes(cIndex)) {
                                    return null;
                                }

                                const isSeat = isSeatSlot(rIndex, cIndex);
                                const data = getSeatContent(rIndex, cIndex);

                                // Aisles are just empty divs (but not null, to keep grid structure if needed, or null if using gap)
                                // Since we use strict grid placement by order, empty divs are needed to push next items if we don't coordinate coords.
                                // But here we map R,C.
                                if (!isSeat) {
                                    return <div key={`${rIndex}-${cIndex}`} className="invisible" />;
                                }

                                const seatColor = data && colorMap && (colorMap[data.student_id!] || colorMap[data.student_name!]);

                                return (
                                    <div
                                        key={`${rIndex}-${cIndex}`}
                                        className={`
                                            relative rounded-md border shadow-sm flex flex-col items-center justify-center p-1 cursor-pointer transition-all hover:scale-105 active:scale-95
                                            ${data ? (seatColor ? 'text-white border-transparent' : 'bg-white border-blue-200 hover:border-blue-400 hover:shadow-md') : 'bg-gray-100 border-gray-200 text-gray-400'}
                                        `}
                                        style={seatColor ? { backgroundColor: isHeatmapMode ? `${seatColor}` : seatColor } : {}}
                                        title={data ? `${data.student_name || data.name} (${data.row + 1},${data.col + 1})` : `空座位 (${rIndex + 1},${cIndex + 1})`}
                                    >
                                        {data ? (
                                            <>
                                                <div className={`w-6 h-6 rounded-full flex items-center justify-center mb-0.5 ${seatColor ? 'bg-white/20 text-white' : 'bg-blue-100 text-blue-600'}`}>
                                                    <User size={14} />
                                                </div>
                                                <span className="text-xs font-medium truncate w-full text-center leading-tight">
                                                    {data.student_name || data.name || data.student_id}
                                                </span>
                                            </>
                                        ) : (
                                            <div className="w-2 h-2 rounded-full bg-gray-300" />
                                        )}
                                    </div>
                                );
                            })
                        ))}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default SeatMap;
