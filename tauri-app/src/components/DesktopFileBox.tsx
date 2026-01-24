import { useState, useEffect, useRef } from 'react';
import { useParams } from 'react-router-dom';
import { getCurrentWebviewWindow } from '@tauri-apps/api/webviewWindow';
import { invoke } from '@tauri-apps/api/core';
import { X, Folder, File, FileText, Image as ImageIcon, Plus, ArrowRight, Layers, LayoutGrid, RotateCcw, Link2, Package, Code, Terminal } from 'lucide-react';
import { listFiles, releaseBoxToDesktop, createFolder, renameFile, openFile, FileItem, getBoxPath } from '../utils/DesktopManager';
import { rename, copyFile, remove, readDir, mkdir, exists, stat, readFile } from '@tauri-apps/plugin-fs';
import { join } from '@tauri-apps/api/path';
import { desktopDir } from '@tauri-apps/api/path';

const DesktopFileBox = () => {
    const { boxId } = useParams();
    const [files, setFiles] = useState<FileItem[]>([]);
    const [loading, setLoading] = useState(false);
    const [contextMenu, setContextMenu] = useState<{ x: number, y: number, file?: FileItem } | null>(null);
    const [isDragOver, setIsDragOver] = useState(false);
    const containerRef = useRef<HTMLDivElement>(null);
    const dropQueueRef = useRef<Promise<void>>(Promise.resolve());
    const [selectedPaths, setSelectedPaths] = useState<string[]>([]);
    const [systemIcons, setSystemIcons] = useState<Record<string, string>>({});
    const systemIconsRef = useRef<Record<string, string>>({});
    const iconFetchTokenRef = useRef(0);
    const [imageThumbs, setImageThumbs] = useState<Record<string, string>>({});
    const imageThumbsRef = useRef<Record<string, string>>({});
    const imageFetchTokenRef = useRef(0);

    const getExt = (name: string) => {
        const parts = name.split('.');
        if (parts.length <= 1) return '';
        return (parts.pop() || '').toLowerCase();
    };

    const isImageFile = (name: string) => /\.(jpg|jpeg|png|gif|webp|bmp)$/i.test(name);

    const getImageMime = (name: string) => {
        const ext = getExt(name);
        switch (ext) {
            case 'jpg':
            case 'jpeg':
                return 'image/jpeg';
            case 'png':
                return 'image/png';
            case 'gif':
                return 'image/gif';
            case 'webp':
                return 'image/webp';
            case 'bmp':
                return 'image/bmp';
            default:
                return 'application/octet-stream';
        }
    };

    const handleDragStart = (e: React.DragEvent, file: FileItem) => {
        // Avoid Chromium/WebView2 download manager and delegate to native drag.
        e.preventDefault();
        e.stopPropagation();
        const isSelected = selectedPaths.includes(file.path);
        const paths = isSelected ? selectedPaths : [file.path];
        if (!isSelected) setSelectedPaths([file.path]);
        invoke('start_file_drag', { paths }).catch((err) => {
            console.error('[FileBox] Native drag failed:', err);
        });
    };

    const handleSelect = (e: React.MouseEvent, file: FileItem) => {
        const additive = e.ctrlKey || e.metaKey;
        setSelectedPaths((prev) => {
            if (!additive) return [file.path];
            if (prev.includes(file.path)) return prev.filter((p) => p !== file.path);
            return [...prev, file.path];
        });
    };

    const getFileIcon = (name: string) => {
        const ext = getExt(name);
        if (['lnk', 'url'].includes(ext)) return { Icon: Link2, className: 'text-sky-500' };
        if (['zip', 'rar', '7z', 'tar', 'gz'].includes(ext)) return { Icon: Package, className: 'text-amber-500' };
        if (['doc', 'docx', 'rtf'].includes(ext)) return { Icon: FileText, className: 'text-blue-500' };
        if (['xls', 'xlsx', 'csv'].includes(ext)) return { Icon: FileText, className: 'text-emerald-600' };
        if (['ppt', 'pptx'].includes(ext)) return { Icon: FileText, className: 'text-orange-500' };
        if (['pdf'].includes(ext)) return { Icon: FileText, className: 'text-red-500' };
        if (['go', 'rs', 'ts', 'tsx', 'js', 'jsx', 'py', 'java', 'cs', 'c', 'cpp', 'h', 'hpp', 'json', 'yml', 'yaml', 'toml', 'md'].includes(ext)) {
            return { Icon: Code, className: 'text-emerald-500' };
        }
        if (['sh', 'bat', 'cmd', 'ps1'].includes(ext)) return { Icon: Terminal, className: 'text-zinc-700' };
        return { Icon: File, className: 'text-blue-400' };
    };

    useEffect(() => {
        systemIconsRef.current = systemIcons;
    }, [systemIcons]);

    useEffect(() => {
        imageThumbsRef.current = imageThumbs;
    }, [imageThumbs]);

    const ensureSystemIcon = async (path: string) => {
        if (systemIconsRef.current[path]) return;
        try {
            const dataUrl = await invoke<string>('get_system_icon', { path, size: 64 });
            setSystemIcons((prev) => (prev[path] ? prev : { ...prev, [path]: dataUrl }));
        } catch {
            // ignore; fallback UI will handle
        }
    };

    useEffect(() => {
        // Prefetch image thumbnails by reading file bytes -> blob URL.
        // This avoids depending on any localhost asset server and matches Explorer-like thumbnails.
        const token = ++imageFetchTokenRef.current;
        const needed = files.filter((f) => isImageFile(f.name)).slice(0, 60);
        const missing = needed.filter((f) => !imageThumbsRef.current[f.path]);
        if (missing.length === 0) return;

        let idx = 0;
        const concurrency = 3;

        const worker = async () => {
            while (idx < missing.length) {
                const cur = missing[idx++];
                if (token !== imageFetchTokenRef.current) return;
                try {
                    const bytes = await readFile(cur.path);
                    if (token !== imageFetchTokenRef.current) return;
                    const blob = new Blob([bytes], { type: getImageMime(cur.name) });
                    const url = URL.createObjectURL(blob);
                    setImageThumbs((prev) => (prev[cur.path] ? prev : { ...prev, [cur.path]: url }));
                } catch (e) {
                    console.warn('[FileBox] Failed to create thumbnail blob URL:', cur.path, e);
                    // fallback to system icon
                    ensureSystemIcon(cur.path);
                }
            }
        };

        for (let i = 0; i < concurrency; i++) worker();
    }, [files]);

    useEffect(() => {
        // Cleanup stale object URLs when file list changes / component unmounts.
        const keep = new Set(files.filter((f) => isImageFile(f.name)).map((f) => f.path));
        const current = imageThumbsRef.current;
        const toRevoke: string[] = [];
        for (const [p, url] of Object.entries(current)) {
            if (!keep.has(p)) toRevoke.push(url);
        }
        if (toRevoke.length) {
            setImageThumbs((prev) => {
                const next = { ...prev };
                for (const p of Object.keys(next)) {
                    if (!keep.has(p)) delete next[p];
                }
                return next;
            });
            for (const url of toRevoke) URL.revokeObjectURL(url);
        }

        return () => {
            for (const url of Object.values(imageThumbsRef.current)) URL.revokeObjectURL(url);
        };
    }, [files]);

    useEffect(() => {
        // Prefetch Windows system icons so UI matches Explorer as closely as possible.
        // Images use thumbnails (convertFileSrc) so we skip them.
        const token = ++iconFetchTokenRef.current;
        const candidates = files.filter((f) => !isImageFile(f.name));
        const missing = candidates
            .filter((f) => !systemIconsRef.current[f.path])
            .slice(0, 80);

        if (missing.length === 0) return;

        let idx = 0;
        const concurrency = 4;

        const worker = async () => {
            while (idx < missing.length) {
                const cur = missing[idx++];
                if (token !== iconFetchTokenRef.current) return;
                try {
                    const dataUrl = await invoke<string>('get_system_icon', { path: cur.path, size: 64 });
                    if (token !== iconFetchTokenRef.current) return;
                    setSystemIcons((prev) => (prev[cur.path] ? prev : { ...prev, [cur.path]: dataUrl }));
                } catch {
                    // ignore; fallback UI will handle
                }
            }
        };

        for (let i = 0; i < concurrency; i++) worker();
    }, [files]);

    const extractDroppedPaths = (event: any): string[] => {
        const payload = event?.payload;
        if (!payload) return [];
        if (Array.isArray(payload)) return payload.filter((p) => typeof p === 'string');
        if (typeof payload === 'string') return [payload];
        const paths = payload?.paths;
        if (Array.isArray(paths)) return paths.filter((p: any) => typeof p === 'string');
        const path = payload?.path;
        if (typeof path === 'string') return [path];
        return [];
    };

    const uniqueDestPath = async (destDir: string, fileName: string) => {
        let candidate = await join(destDir, fileName);
        if (!(await exists(candidate))) return candidate;

        const hasExt = fileName.includes('.') && !fileName.endsWith('.');
        const ext = hasExt ? `.${fileName.split('.').pop()}` : '';
        const nameNoExt = hasExt ? fileName.split('.').slice(0, -1).join('.') : fileName;
        candidate = await join(destDir, `${nameNoExt}_${Date.now()}${ext}`);
        return candidate;
    };

    const copyDirRecursive = async (srcDir: string, destDir: string) => {
        await mkdir(destDir, { recursive: true });
        const entries = await readDir(srcDir);
        for (const entry of entries) {
            const srcChild = await join(srcDir, entry.name);
            const destChild = await join(destDir, entry.name);
            if (entry.isDirectory) {
                await copyDirRecursive(srcChild, destChild);
            } else {
                await copyFile(srcChild, destChild);
            }
        }
    };

    const moveItemToDesktop = async (item: FileItem, mode: 'move' | 'copy') => {
        const desktop = await desktopDir();
        const destPath = await uniqueDestPath(desktop, item.name);
        try {
            if (mode === 'move') {
                try {
                    await rename(item.path, destPath);
                } catch (e) {
                    // cross-device or permission issues: fallback to copy+delete
                    const meta = await stat(item.path);
                    if (meta.isDirectory) await copyDirRecursive(item.path, destPath);
                    else await copyFile(item.path, destPath);
                    await remove(item.path, { recursive: true });
                }
            } else {
                const meta = await stat(item.path);
                if (meta.isDirectory) await copyDirRecursive(item.path, destPath);
                else await copyFile(item.path, destPath);
            }
            await loadFiles();
        } catch (e) {
            console.error('[FileBox] Failed to move/copy item to desktop', e);
            alert(`操作失败：${item.name}\n错误：${JSON.stringify(e)}`);
        }
    };

    const loadFiles = async () => {
        if (!boxId) return;
        setLoading(true);
        try {
            const items = await listFiles(boxId);
            setFiles(items);
        } catch (e) {
            console.error(e);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        console.log(`[FileBox] Mounted for boxId: ${boxId}`);
        loadFiles();

        let unlistenDrop: (() => void) | undefined;
        let unlistenDragDrop: (() => void) | undefined;
        let unlistenDragEnter: (() => void) | undefined;
        let unlistenDragLeave: (() => void) | undefined;
        let isActive = true;

        // Global HTML5 Drag & Drop listeners (Bypass React)
        const onWindowDragOver = (e: DragEvent) => {
            e.preventDefault();
            if (e.dataTransfer) e.dataTransfer.dropEffect = 'copy';
            setIsDragOver(true);
        };

        const onWindowDrop = (e: DragEvent) => {
            e.preventDefault();
            console.log('[FileBox] Global HTML5 Window Drop event fired');
            setIsDragOver(false);
        };

        window.addEventListener('dragover', onWindowDragOver);
        window.addEventListener('drop', onWindowDrop);

        const setupListeners = async () => {
            const appWindow = getCurrentWebviewWindow();

            // Listen for file drops
            console.log('[FileBox] Setting up drop listener...');

            const handleDropPayloadInner = async (event: any, source: string) => {
                console.log(`[FileBox] Drop event received (${source}):`, event);
                setIsDragOver(false);
                if (!boxId) {
                    console.error('[FileBox] No boxId found in drop handler');
                    return;
                }
                const paths = Array.from(new Set(extractDroppedPaths(event)));
                console.log('[FileBox] Dropped paths:', paths);

                if (paths && paths.length > 0) {
                    try {
                        const boxPath = await getBoxPath(boxId);
                        console.log('[FileBox] Target box path:', boxPath);

                        for (const srcPath of paths) {
                            console.log(`[FileBox] Processing file: ${srcPath}`);
                            const fileName = srcPath.split(/[/\\]/).pop();
                            if (fileName) {
                                try {
                                    const destPath = await uniqueDestPath(boxPath, fileName);
                                    console.log(`[FileBox] Destination path: ${destPath}`);

                                    // Try rename first
                                    try {
                                        console.log('[FileBox] Attempting rename...');
                                        await rename(srcPath, destPath);
                                        console.log('[FileBox] Rename successful');
                                    } catch (renameErr) {
                                        // If we got double-fired, the first handler may have already moved it.
                                        if (!(await exists(srcPath))) {
                                            console.warn(`[FileBox] Source path no longer exists, likely already moved: ${srcPath}`);
                                            continue;
                                        }
                                        console.warn(`[FileBox] Rename failed for ${srcPath}, trying copy+delete...`, renameErr);
                                        const meta = await stat(srcPath);
                                        if (meta.isDirectory) {
                                            console.log('[FileBox] Source is directory, attempting recursive copy...');
                                            await copyDirRecursive(srcPath, destPath);
                                        } else {
                                            console.log('[FileBox] Attempting copyFile...');
                                            await copyFile(srcPath, destPath);
                                        }
                                        console.log('[FileBox] Copy successful, deleting original...');
                                        await remove(srcPath, { recursive: true });
                                        console.log('[FileBox] Original deleted');
                                    }
                                } catch (e) {
                                    console.error(`[FileBox] Failed to move ${srcPath}`, e);
                                    alert(`移动文件失败: ${fileName}\n错误: ${JSON.stringify(e)}`);
                                }
                            }
                        }
                        console.log('[FileBox] All files processed, reloading...');
                        loadFiles();
                    } catch (err) {
                        console.error('[FileBox] Unexpected error in drop handler:', err);
                    }
                } else {
                    console.log('[FileBox] No paths in payload');
                }
            };

            const enqueue = (event: any, source: string) => {
                dropQueueRef.current = dropQueueRef.current
                    .then(() => (isActive ? handleDropPayloadInner(event, source) : undefined))
                    .catch((e) => console.error('[FileBox] Drop queue error:', e));
            };

            // Tauri v2 event name
            const u1 = await appWindow.listen('tauri://drag-drop', (event: any) => enqueue(event, 'tauri://drag-drop'));
            if (!isActive) u1();
            else unlistenDragDrop = u1;

            // Backwards/variant compatibility (some codebases used a shorter alias)
            const u2 = await appWindow.listen('tauri://drop', (event: any) => enqueue(event, 'tauri://drop'));
            if (!isActive) u2();
            else unlistenDrop = u2;

            const u3 = await appWindow.listen('tauri://drag-enter', () => {
                console.log('[FileBox] Drag enter');
                setIsDragOver(true);
            });
            if (!isActive) u3();
            else unlistenDragEnter = u3;

            const u4 = await appWindow.listen('tauri://drag-leave', () => {
                console.log('[FileBox] Drag leave');
                setIsDragOver(false);
            });
            if (!isActive) u4();
            else unlistenDragLeave = u4;

            console.log('[FileBox] Listeners set up.');
        };

        setupListeners();

        // Auto-refresh interval
        const interval = setInterval(loadFiles, 2000);

        return () => {
            isActive = false;
            if (unlistenDrop) unlistenDrop();
            if (unlistenDragDrop) unlistenDragDrop();
            if (unlistenDragEnter) unlistenDragEnter();
            if (unlistenDragLeave) unlistenDragLeave();
            window.removeEventListener('dragover', onWindowDragOver);
            window.removeEventListener('drop', onWindowDrop);
            clearInterval(interval);
        };
    }, [boxId]);

    const handleRelease = async () => {
        if (!boxId) return;
        if (confirm('确定要收起此文件盒子吗？所有文件将移动回桌面。')) {
            await releaseBoxToDesktop(boxId);
            await getCurrentWebviewWindow().close();
        }
    };

    const handleContextMenu = (e: React.MouseEvent, file?: FileItem) => {
        e.preventDefault();
        setContextMenu({ x: e.clientX, y: e.clientY, file });
    };

    const handleNewFolder = async () => {
        if (!boxId) return;
        const name = prompt('请输入文件夹名称', '新建文件夹');
        if (name) {
            await createFolder(boxId, name);
            loadFiles();
        }
        setContextMenu(null);
    };

    const handleRename = async () => {
        if (!contextMenu?.file) return;
        const newName = prompt('重命名', contextMenu.file.name);
        if (newName && newName !== contextMenu.file.name) {
            try {
                await renameFile(contextMenu.file.path, newName);
                loadFiles();
            } catch (e) {
                alert('重命名失败');
                console.error(e);
            }
        }
        setContextMenu(null);
    };

    const handleOpenFile = async (file: FileItem) => {
        try {
            await openFile(file.path);
        } catch (e) {
            console.error("Failed to open file", e);
        }
    };

    return (
        <div
            className={`h-screen w-screen flex flex-col overflow-hidden select-none transition-all duration-300 ${isDragOver ? 'bg-blue-50/95 ring-4 ring-inset ring-blue-400' : 'bg-white/90'} backdrop-blur-2xl shadow-[0_0_40px_rgba(0,0,0,0.1)] rounded-xl border border-white/20 text-gray-800`}
            onContextMenu={(e) => handleContextMenu(e)}
            onClick={() => { setContextMenu(null); setSelectedPaths([]); }}
            onDragOver={(e) => {
                e.preventDefault(); // Necessary to allow dropping
                e.dataTransfer.dropEffect = 'copy';
                if (!isDragOver) setIsDragOver(true);
            }}
            onDragLeave={(e) => {
                // Check if leaving the window or just entering a child
                if (e.relatedTarget === null || e.currentTarget.contains(e.relatedTarget as Node) === false) {
                    setIsDragOver(false);
                }
            }}
            onDrop={(e) => {
                e.preventDefault();
                console.log('[FileBox] HTML5 Drop event fired');
                // We rely on 'tauri://drop' for the paths, but this prevents browser navigation
                setIsDragOver(false);
            }}
        >
            {/* Header / Drag Region */}
            <div data-tauri-drag-region className="flex items-center justify-between px-4 py-3 bg-gradient-to-b from-white/50 to-white/10 border-b border-gray-200/50 cursor-move shrink-0">
                <div className="flex items-center gap-3 pointer-events-none">
                    <div className="flex gap-1.5">
                        <span className="w-3 h-3 rounded-full bg-red-400 hover:bg-red-500 transition-colors shadow-sm"></span>
                        <span className="w-3 h-3 rounded-full bg-yellow-400 hover:bg-yellow-500 transition-colors shadow-sm"></span>
                        <span className="w-3 h-3 rounded-full bg-green-400 hover:bg-green-500 transition-colors shadow-sm"></span>
                    </div>
                    <div className="h-4 w-[1px] bg-gray-300 mx-1"></div>
                    <span className="text-xs font-semibold text-gray-600 flex items-center gap-1.5">
                        <Layers size={14} className="text-blue-500" />
                        文件盒子
                    </span>
                </div>
                <div className="flex items-center gap-1">
                    <button
                        onClick={loadFiles}
                        className="p-1.5 hover:bg-black/5 rounded-lg text-gray-400 hover:text-blue-500 transition-colors"
                        title="刷新"
                    >
                        <RotateCcw size={14} />
                    </button>
                    <button
                        onClick={handleRelease}
                        className="flex items-center gap-1.5 px-2.5 py-1.5 hover:bg-red-50 text-xs font-medium text-gray-500 hover:text-red-500 rounded-lg transition-all group"
                        title="释放到桌面"
                    >
                        <span>释放</span>
                        <ArrowRight size={14} className="group-hover:translate-x-0.5 transition-transform" />
                    </button>
                    <button
                        onClick={() => getCurrentWebviewWindow().close()}
                        className="p-1.5 hover:bg-red-100/50 rounded-lg text-gray-400 hover:text-red-600 transition-colors ml-1"
                    >
                        <X size={16} />
                    </button>
                </div>
            </div>

            {/* Content Grid */}
            <div
                ref={containerRef}
                className="flex-1 overflow-y-auto p-5 content-start grid grid-cols-4 gap-4 align-content-start scrollbar-hide"
                style={{ scrollbarWidth: 'none', msOverflowStyle: 'none' }}
            >
                <style>{`
                    ::-webkit-scrollbar { display: none; }
                `}</style>

                {isDragOver && (
                    <div className="col-span-4 flex flex-col items-center justify-center h-48 border-2 border-dashed border-blue-400/50 rounded-2xl bg-blue-50/50 animate-in fade-in zoom-in-95 duration-200">
                        <Plus size={32} className="text-blue-500 mb-2" />
                        <span className="text-base font-medium text-blue-600">释放以添加文件</span>
                    </div>
                )}

                {!loading && files.length === 0 && !isDragOver && (
                    <div className="col-span-4 flex flex-col items-center justify-center h-52 opacity-40 space-y-3 pointer-events-none select-none">
                        <div className="w-16 h-16 rounded-full bg-gray-100 flex items-center justify-center">
                            <LayoutGrid size={32} className="text-gray-400" />
                        </div>
                        <span className="text-sm font-medium text-gray-500">暂无内容</span>
                        <span className="text-xs text-gray-400">拖入文件或右键新建</span>
                    </div>
                )}

                {files.map(file => (
                    <div
                        key={file.name}
                        className={`flex flex-col items-center gap-2 group cursor-pointer p-3 rounded-xl transition-all duration-200 active:scale-95 ring-1 ${
                            selectedPaths.includes(file.path)
                                ? 'bg-blue-50 ring-blue-300 shadow-lg shadow-blue-100/40'
                                : 'hover:bg-white hover:shadow-lg hover:shadow-gray-200/50 ring-transparent hover:ring-gray-100'
                        }`}
                        onDoubleClick={() => handleOpenFile(file)}
                        onContextMenu={(e) => {
                            e.stopPropagation();
                            if (!selectedPaths.includes(file.path)) setSelectedPaths([file.path]);
                            handleContextMenu(e, file);
                        }}
                        onClick={(e) => { e.stopPropagation(); handleSelect(e, file); }}
                        draggable
                        onDragStart={(e) => handleDragStart(e, file)}
                    >
                        <div className="w-14 h-14 flex items-center justify-center transition-transform duration-200 group-hover:-translate-y-1">
                            {isImageFile(file.name) ? (
                                <div className="relative w-12 h-12">
                                    {imageThumbs[file.path] ? (
                                        <img
                                            src={imageThumbs[file.path]}
                                            className="w-12 h-12 object-cover rounded-xl shadow-sm ring-1 ring-black/5 bg-white"
                                            alt={file.name}
                                        />
                                    ) : systemIcons[file.path] ? (
                                        <img src={systemIcons[file.path]} className="w-12 h-12 object-contain drop-shadow-sm" alt={file.name} />
                                    ) : (
                                        <ImageIcon className="text-purple-500 drop-shadow-sm" size={44} />
                                    )}
                                </div>
                            ) : (
                                systemIcons[file.path] ? (
                                    <img
                                        src={systemIcons[file.path]}
                                        className="w-12 h-12 object-contain drop-shadow-sm"
                                        alt={file.name}
                                    />
                                ) : (() => {
                                    const { Icon, className } = getFileIcon(file.name);
                                    const ext = getExt(file.name);
                                    return (
                                        <div className="relative">
                                            {file.isDir ? (
                                                <Folder className="text-yellow-400 drop-shadow-sm filter" size={48} fill="currentColor" />
                                            ) : (
                                                <Icon className={`${className} drop-shadow-sm`} size={44} />
                                            )}
                                            {ext && (
                                                <div className="absolute -bottom-1 -right-1 bg-white/95 backdrop-blur rounded-full px-1.5 py-0.5 shadow-sm ring-1 ring-black/5">
                                                    <span className="text-[9px] font-bold text-gray-700">{ext.toUpperCase()}</span>
                                                </div>
                                            )}
                                        </div>
                                    );
                                })()
                            )}
                        </div>
                        <span className="text-xs font-medium text-center text-gray-600 w-full truncate px-2 py-0.5 rounded group-hover:text-gray-900 transition-colors" title={file.name}>
                            {file.name}
                        </span>
                    </div>
                ))}
            </div>

            {/* Custom Context Menu */}
            {contextMenu && (
                <div
                    className="fixed z-50 bg-white/90 backdrop-blur-xl border border-gray-100/50 rounded-xl shadow-[0_4px_20px_rgba(0,0,0,0.08)] py-1.5 min-w-[150px] animate-in fade-in zoom-in-95 duration-100 ring-1 ring-black/5"
                    style={{ left: contextMenu.x, top: contextMenu.y }}
                    onClick={(e) => e.stopPropagation()}
                >
                    {contextMenu.file ? (
                        <>
                            <button onClick={handleOpenFile.bind(null, contextMenu.file)} className="w-full text-left px-3 py-2 text-sm text-gray-700 hover:bg-blue-50 hover:text-blue-600 flex items-center gap-2.5 transition-colors font-medium">
                                <File size={15} /> 打开
                            </button>
                            <button
                                onClick={async () => {
                                    await moveItemToDesktop(contextMenu.file!, 'move');
                                    setContextMenu(null);
                                }}
                                className="w-full text-left px-3 py-2 text-sm text-gray-700 hover:bg-blue-50 hover:text-blue-600 flex items-center gap-2.5 transition-colors font-medium"
                            >
                                <ArrowRight size={15} /> 移动到桌面
                            </button>
                            <button
                                onClick={async () => {
                                    await moveItemToDesktop(contextMenu.file!, 'copy');
                                    setContextMenu(null);
                                }}
                                className="w-full text-left px-3 py-2 text-sm text-gray-700 hover:bg-blue-50 hover:text-blue-600 flex items-center gap-2.5 transition-colors font-medium"
                            >
                                <ArrowRight size={15} /> 复制到桌面
                            </button>
                            <div className="h-[1px] bg-gray-100 my-1 mx-2"></div>
                            <button onClick={handleRename} className="w-full text-left px-3 py-2 text-sm text-gray-700 hover:bg-blue-50 hover:text-blue-600 flex items-center gap-2.5 transition-colors font-medium">
                                <FileText size={15} /> 重命名
                            </button>
                        </>
                    ) : (
                        <button onClick={handleNewFolder} className="w-full text-left px-3 py-2 text-sm text-gray-700 hover:bg-blue-50 hover:text-blue-600 flex items-center gap-2.5 transition-colors font-medium">
                            <Plus size={15} /> 新建文件夹
                        </button>
                    )}
                </div>
            )}
        </div>
    );
};

export default DesktopFileBox;
