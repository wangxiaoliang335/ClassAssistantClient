import { mkdir, readDir, remove, rename, BaseDirectory, exists, stat } from '@tauri-apps/plugin-fs';
import { open } from '@tauri-apps/plugin-shell';
import { join, desktopDir, documentDir, sep } from '@tauri-apps/api/path';

const BOX_BASE_DIR = 'EduDesk/Boxes';

export interface FileBoxInfo {
    id: string;
    path: string;
    name: string;
}

export interface FileItem {
    name: string;
    path: string;
    isDir: boolean;
    size?: number;
}

// Ensure the base directory exists: /Documents/EduDesk/Boxes
const ensureBaseDir = async () => {
    const base = await join(await documentDir(), 'EduDesk', 'Boxes');
    if (!(await exists(base))) {
        await mkdir(base, { recursive: true });
    }
    return base;
};

export const createNewBox = async (): Promise<FileBoxInfo> => {
    await ensureBaseDir();
    const boxId = `box_${Date.now()}`;
    const boxPath = await join(await documentDir(), 'EduDesk', 'Boxes', boxId);

    await mkdir(boxPath);

    return {
        id: boxId,
        path: boxPath,
        name: '新建盒子'
    };
};

export const getBoxPath = async (boxId: string): Promise<string> => {
    return await join(await documentDir(), 'EduDesk', 'Boxes', boxId);
};

export const listFiles = async (boxId: string): Promise<FileItem[]> => {
    const path = await getBoxPath(boxId);
    if (!await exists(path)) return [];

    const entries = await readDir(path);
    const items: FileItem[] = [];

    for (const entry of entries) {
        items.push({
            name: entry.name,
            path: await join(path, entry.name),
            isDir: entry.isDirectory,
        });
    }
    return items;
};

// "Release" the box: Move all containing files to Desktop, then delete the box folder.
export const releaseBoxToDesktop = async (boxId: string): Promise<void> => {
    const boxPath = await getBoxPath(boxId);
    if (!await exists(boxPath)) return;

    const desktop = await desktopDir();
    const entries = await readDir(boxPath);

    for (const entry of entries) {
        const srcPath = await join(boxPath, entry.name);
        const destPath = await join(desktop, entry.name);

        // Handle name collision by appending timestamp if needed, or simple rename
        // For simplicity here, we try direct move. If collision, fs might error or overwrite depending on OS.
        // Better: check exists and rename.
        let finalDest = destPath;
        if (await exists(finalDest)) {
            const ext = entry.name.includes('.') ? `.${entry.name.split('.').pop()}` : '';
            const nameNoExt = entry.name.includes('.') ? entry.name.split('.').slice(0, -1).join('.') : entry.name;
            finalDest = await join(desktop, `${nameNoExt}_${Date.now()}${ext}`);
        }

        try {
            await rename(srcPath, finalDest);
        } catch (e) {
            console.error(`Failed to move ${entry.name} to desktop:`, e);
        }
    }

    // Now remove the empty box directory
    try {
        await remove(boxPath, { recursive: true });
    } catch (e) {
        console.error("Failed to remove box dir:", e);
    }
};

export const createFolder = async (boxId: string, folderName: string): Promise<void> => {
    const boxPath = await getBoxPath(boxId);
    const newDirPath = await join(boxPath, folderName);
    if (!await exists(newDirPath)) {
        await mkdir(newDirPath);
    }
};

export const renameFile = async (oldPath: string, newName: string): Promise<void> => {
    // Need to handle getting parent dir from oldPath
    // This helper might need more robust path handling
    const parent = oldPath.substring(0, oldPath.lastIndexOf(sep())); // Primitive, assumes separator
    // Better to use path API if possible, for now assume we pass full oldPath
    // Actually, let's look for last index of / or \

    // Simplification: We assume oldPath is valid.
    // We need to construct new path.
    // Since we don't have 'dirname' in path api easily exposed synchronously here without awaiting, 
    // we might accept parent path as arg or regex split.

    // Hacky split for now:
    const parts = oldPath.split(/[/\\]/);
    parts.pop();
    const parentDir = parts.join(await sep());
    const newPath = await join(parentDir, newName);

    await rename(oldPath, newPath);
};

export const openFile = async (path: string) => {
    await open(path);
};
