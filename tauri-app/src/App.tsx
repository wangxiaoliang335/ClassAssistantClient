import { useState } from "react";
import { BrowserRouter, Routes, Route } from "react-router-dom";
import ClassLogin from "./components/ClassLogin";
import Dashboard from "./components/Dashboard";
import ClassChatWindow from "./components/ClassChatWindow";
import ClassScheduleWindow from "./components/ClassScheduleWindow";
import IntercomWindow from "./components/IntercomWindow";
import NormalGroupChatWindow from "./components/NormalGroupChatWindow";
import DesktopFileBox from "./components/DesktopFileBoxFixed";

import { invoke } from "@tauri-apps/api/core";
import { loginTIM, getTIMGroups, setCachedTIMGroups } from "./utils/tim";

// Main App Component handling Login/Dashboard flow
function MainApp() {
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [userInfo, setUserInfo] = useState<any>(null);

  const handleLoginSuccess = async (data: any) => {
    // Save token for API calls
    const token = data.token || data.data?.token || data.access_token || data.data?.access_token || '';
    if (token) localStorage.setItem('token', token);

    try {
      await invoke('resize_window');

      // Fetch full user info
      let phone = data.phone || data.data?.phone;
      let userId = data.id || data.data?.id;

      if (!phone && !userId && data.userinfo && Array.isArray(data.userinfo) && data.userinfo.length > 0) {
        phone = data.userinfo[0].phone;
        userId = data.userinfo[0].id;
      }

      if (phone || userId) {
        try {
          const infoResStr = await invoke<string>('get_user_info', {
            phone: phone,
            userId: userId ? String(userId) : null,
            token: token
          });
          const infoRes = JSON.parse(infoResStr);
          if (infoRes.data?.code === 200 && infoRes.data?.userinfo?.length > 0) {
            const fullInfo = infoRes.data.userinfo[0];
            setUserInfo(fullInfo);
            setIsLoggedIn(true);

            if (fullInfo.teacher_unique_id) {
              // WebSocket 连接已由 WebSocketProvider 统一管理，此处无需重复调用 connectWS

              localStorage.setItem('teacher_unique_id', fullInfo.teacher_unique_id);
              if (fullInfo.id_number) localStorage.setItem('id_number', fullInfo.id_number);
              if (fullInfo.name) localStorage.setItem('name', fullInfo.name);

              // Fetch UserSig for TIM
              try {
                const sig = await invoke<string>('get_user_sig', {
                  userId: fullInfo.teacher_unique_id
                });
                if (sig) {
                  localStorage.setItem('userSig', sig);

                  // Login to TIM and cache groups early so CreateClassGroupModal can use them
                  const timLoginSuccess = await loginTIM(fullInfo.teacher_unique_id, sig);
                  if (timLoginSuccess) {
                    const timGroups = await getTIMGroups();
                    setCachedTIMGroups(timGroups);
                  }
                }
              } catch (e) {
                console.error('Failed to fetch UserSig or login TIM:', e);
              }
              return;
            }
          }
        } catch (err) {
          console.error('Failed to fetch user info:', err);
        }
      }
    } catch (e) {
      console.error('Failed to resize window or fetch info:', e);
    }

    setUserInfo(data);
    setIsLoggedIn(true);

    // 如果是班级登录，自动刷新班级信息
    if (data.loginType === 'class' && data.class_code) {
      try {
        const classCode = data.class_code;
        const token = data.access_token || data.token || '';
        
        const classInfoResStr = await invoke<string>('get_class_info', {
          classCode,
          token
        });
        const classInfoRes = JSON.parse(classInfoResStr);
        const classData = classInfoRes?.data || classInfoRes;
        
        if (Number(classData?.code) === 200) {
          const updatedClassInfo = {
            ...data,
            class_code: classData.class_code || classCode,
            class_name: classData.class_name || data.class_name,
            school_stage: classData.school_stage || data.school_stage,
            grade: classData.grade || data.grade,
            schoolid: classData.schoolid || data.schoolid,
            school_name: classData.school_name || data.school_name,
            address: classData.address || data.address,
            face_url: classData.face_url || data.face_url
          };
          
          setUserInfo(updatedClassInfo);
          localStorage.setItem('user_info', JSON.stringify(updatedClassInfo));
        }
      } catch (err) {
      }
    }
  };

  return (
    <div className="h-screen w-screen overflow-hidden text-white selection:bg-blue-500 selection:text-white">
      {!isLoggedIn ? (
        <ClassLogin onLoginSuccess={handleLoginSuccess} />
      ) : (
        <Dashboard userInfo={userInfo} />
      )}
    </div>
  );
}

// Root App Component dealing with Routing
function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<MainApp />} />
        <Route path="/class/schedule/:groupclassId" element={<ClassScheduleWindow />} />
        <Route path="/class/chat/:groupclassId" element={<ClassChatWindow />} />
        <Route path="/intercom/:groupId" element={<IntercomWindow />} />
        <Route path="/chat/normal/:groupId" element={<NormalGroupChatWindow />} />
        <Route path="/file-box/:boxId" element={<DesktopFileBox />} />
      </Routes>
    </BrowserRouter>
  );
}

export default App;

