// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
use tauri::Manager;
use serde::Serialize;

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[tauri::command]
async fn login(phone: &str, password: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let res = client.post("http://47.100.126.194:5000/login")
        .json(&serde_json::json!({
            "phone": phone,
            "password": password
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn class_login(class_number: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let res = client.post("http://47.100.126.194:5000/login")
        .json(&serde_json::json!({
            "class_number": class_number,
            "login_type": "class"
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn send_verification_code(phone: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    // Logic from RegisterDialog.cpp/ResetPwdDialog.cpp
    let res = client.post("http://47.100.126.194:5000/send_verification_code")
        .form(&serde_json::json!({
            "phone": phone
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn register_account(phone: &str, password: &str, verification_code: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    // Logic from RegisterDialog.cpp
    let res = client.post("http://47.100.126.194:5000/register")
        .form(&serde_json::json!({
            "phone": phone,
            "password": password,
            "verification_code": verification_code
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn reset_password(phone: &str, new_password: &str, verification_code: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    // Logic from ResetPwdDialog.cpp
    let res = client.post("http://47.100.126.194:5000/verify_and_set_password")
        .form(&serde_json::json!({
            "phone": phone,
            "new_password": new_password,
            "verification_code": verification_code
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = res.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn resize_window(window: tauri::Window) {
    let _ = window.set_resizable(true);
    let _ = window.set_size(tauri::Size::Physical(tauri::PhysicalSize { width: 1280, height: 800 }));
    let _ = window.center();
}

#[tauri::command]
async fn get_course_schedule(class_id: &str, term: Option<&str>, token: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let mut url = format!("http://47.100.126.194:5000/course-schedule?class_id={}", class_id);
    
    if let Some(t) = term {
        url.push_str(&format!("&term={}", t));
    }

    let res = client.get(&url)
        .header("Authorization", token) // Assuming token is passed directly or as Bearer? User said "Token: needed in header". Usually implies "Authorization". Qt code might clarify but let's try direct or Bearer. I'll use raw token first as Qt usually sends raw or specific format.
        .send()
        .await
        .map_err(|e| e.to_string())?;

    if res.status().is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", res.status()))
    }
}

#[tauri::command]
async fn get_user_friends(id_card: &str, token: &str) -> Result<String, String> {
    println!("Backend: get_user_friends called with id_card: {}", id_card);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/friends?id_card={}", id_card);

    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_user_friends status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        // println!("Backend: get_user_friends response: {}", text); // Optional: verbose
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn get_teacher_classes(teacher_unique_id: &str, token: &str) -> Result<String, String> {
    println!("Backend: get_teacher_classes called with teacher_unique_id: {}", teacher_unique_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/teachers/classes?teacher_unique_id={}", teacher_unique_id);

    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let status = res.status();
    println!("Backend: get_teacher_classes status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn get_user_info(phone: Option<&str>, user_id: Option<&str>, token: &str) -> Result<String, String> {
    println!("Backend: get_user_info called. Phone: {:?}, UserId: {:?}", phone, user_id);
    let client = reqwest::Client::new();
    let mut url = "http://47.100.126.194:5000/userInfo?".to_string();
    
    if let Some(p) = phone {
        url.push_str(&format!("phone={}", p));
    } else if let Some(u) = user_id {
        url.push_str(&format!("userid={}", u));
    }

    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_user_info status: {}", status);

    if status.is_success() {
        Ok(res.text().await.map_err(|e| e.to_string())?)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn get_user_sig(user_id: &str) -> Result<String, String> {
    println!("Backend: get_user_sig called for user_id: {}", user_id);
    let client = reqwest::Client::new();
    let res = client.post("http://47.100.126.194:5000/getUserSig")
        .form(&serde_json::json!({
            "user_id": user_id
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_user_sig status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        // Parse the JSON to extract the user_sig field
        let json: serde_json::Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
        
        // C++ checks multiple fields: user_sig, usersig, sig
        let sig = json["data"]["user_sig"].as_str()
            .or_else(|| json["data"]["usersig"].as_str())
            .or_else(|| json["data"]["sig"].as_str())
            .or_else(|| json["user_sig"].as_str())
            .or_else(|| json["usersig"].as_str())
            .or_else(|| json["sig"].as_str());

        if let Some(s) = sig {
            println!("Backend: UserSig found");
            Ok(s.to_string())
        } else {
            println!("Backend: UserSig NOT found in response: {}", text);
            Err("UserSig not found in response".to_string())
        }
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}

#[tauri::command]
async fn open_class_window(app: tauri::AppHandle, groupclass_id: String) -> Result<(), String> {
    println!("Backend: Opening class schedule window for ID: {}", groupclass_id);
    
    let window_label = format!("class_schedule_{}", groupclass_id);
    // Route to the new Schedule Window
    let url = format!("/class/schedule/{}", groupclass_id);

    if let Some(window) = app.get_webview_window(&window_label) {
        let _ = window.set_focus();
        return Ok(());
    }

    let builder = tauri::WebviewWindowBuilder::new(
        &app,
        window_label,
        tauri::WebviewUrl::App(url.into()),
    )
    .title("班级空间")
    .inner_size(1280.0, 800.0)
    .decorations(false)
    .transparent(true);

    match builder.build() {
        Ok(_) => Ok(()),
        Err(e) => Err(format!("Failed to create window: {}", e)),
    }
}

#[tauri::command]
async fn open_chat_window(app: tauri::AppHandle, groupclass_id: String) -> Result<(), String> {
    println!("Backend: Opening class chat window for ID: {}", groupclass_id);
    
    let window_label = format!("class_chat_{}", groupclass_id);
    // Route to the Chat Window
    let url = format!("/class/chat/{}", groupclass_id);

    if let Some(window) = app.get_webview_window(&window_label) {
        let _ = window.set_focus();
        return Ok(());
    }

    let builder = tauri::WebviewWindowBuilder::new(
        &app,
        window_label,
        tauri::WebviewUrl::App(url.into()),
    )
    .title("班级群")
    .inner_size(800.0, 600.0)
    .decorations(false)
    .transparent(true);

    match builder.build() {
        Ok(_) => Ok(()),
        Err(e) => Err(format!("Failed to create window: {}", e)),
    }
}

#[tauri::command]
async fn open_file_box_window(app: tauri::AppHandle, box_id: String) -> Result<(), String> {
    println!("Backend: Opening file box window for ID: {}", box_id);

    let window_label = format!("file_box_{}", box_id);
    let url = format!("/file-box/{}", box_id);

    if let Some(window) = app.get_webview_window(&window_label) {
        let _ = window.set_focus();
        return Ok(());
    }

    let builder = tauri::WebviewWindowBuilder::new(
        &app,
        window_label,
        tauri::WebviewUrl::App(url.into()),
    )
    .title("文件盒子")
    .inner_size(900.0, 650.0)
    .decorations(false)
    .transparent(true);

    match builder.build() {
        Ok(_) => Ok(()),
        Err(e) => Err(format!("Failed to create window: {}", e)),
    }
}

#[tauri::command]
async fn get_group_members(group_id: &str, token: &str) -> Result<String, String> {
    println!("Backend: get_group_members called for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/groups/members?group_id={}", group_id);
    
    let res = client.get(&url)
        .header("Authorization", token)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    println!("Backend: get_group_members status: {}", status);

    if status.is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}", status))
    }
}



#[tauri::command]
async fn fetch_seat_map(class_id: String) -> Result<String, String> {
    println!("Backend: Fetching seat map for class_id: {}", class_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/seat-arrangement";
    
    let response = client
        .get(url)
        .query(&[("class_id", &class_id)])
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn save_seat_map(class_id: String, seats: Vec<serde_json::Value>) -> Result<String, String> {
    println!("Backend: Saving seat map for class_id: {}", class_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/seat-arrangement/save";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "class_id": class_id,
            "seats": seats
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn save_teach_subjects(group_id: String, user_id: String, teach_subjects: Vec<String>) -> Result<String, String> {
    println!("Backend: Saving teach subjects for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/groups/member/teach-subjects";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "user_id": user_id,
            "teach_subjects": teach_subjects
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn toggle_group_intercom(group_id: String, enable: bool) -> Result<String, String> {
    println!("Backend: Toggling intercom for group_id: {}, enable: {}", group_id, enable);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/groups/intercom/toggle";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "enable_intercom": enable
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn fetch_temp_room(group_id: String) -> Result<String, String> {
    println!("Backend: Fetching temp room for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/temp_rooms/query";

    let response = client
        .post(url)
        .json(&serde_json::json!({
            "group_ids": [group_id]
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
    
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn open_intercom_window(app: tauri::AppHandle, group_id: String) -> Result<(), String> {
    println!("Backend: Opening intercom window for ID: {}", group_id);
    
    let window_label = format!("intercom_{}", group_id);
    let url = format!("/intercom/{}", group_id);

    if let Some(window) = app.get_webview_window(&window_label) {
        let _ = window.set_focus();
        return Ok(());
    }

    let builder = tauri::WebviewWindowBuilder::new(
        &app,
        window_label,
        tauri::WebviewUrl::App(url.into()),
    )
    .title("对讲")
    .inner_size(800.0, 500.0) 
    .decorations(false)
    .transparent(true);

    match builder.build() {
        Ok(_) => Ok(()),
        Err(e) => Err(format!("Failed to create window: {}", e)),
    }
}

#[tauri::command]
async fn fetch_duty_roster(group_id: String) -> Result<String, String> {
    println!("Backend: Fetching duty roster for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/duty-roster?group_id={}", group_id);
    let response = client.get(&url).send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn save_duty_roster(group_id: String, rows: Vec<Vec<String>>, requirement_row_index: i32) -> Result<String, String> {
    println!("Backend: Saving duty roster for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/duty-roster";
    let response = client.post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "rows": rows,
            "requirement_row_index": requirement_row_index
        }))
        .send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn fetch_class_wallpapers(group_id: String) -> Result<String, String> {
    println!("Backend: Fetching class wallpapers for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("http://47.100.126.194:5000/class-wallpapers?group_id={}", group_id);
    let response = client.get(&url).send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn fetch_wallpaper_library() -> Result<String, String> {
    println!("Backend: Fetching wallpaper library");
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/wallpaper-library";
    let response = client.get(url).send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn set_class_wallpaper(group_id: String, wallpaper_id: i32) -> Result<String, String> {
    println!("Backend: Setting class wallpaper group_id: {}, wallpaper_id: {}", group_id, wallpaper_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/class-wallpapers/set-current";
    let response = client.post(url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "wallpaper_id": wallpaper_id
        }))
        .send().await.map_err(|e| e.to_string())?;
    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn fetch_weekly_config(group_id: String) -> String {
    println!("Backend: Fetching weekly config for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("{}class-wallpapers/weekly-config?group_id={}", API_BASE_URL, group_id);
    
    match client.get(&url).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => {
                    println!("Backend: Weekly config response: {}", text);
                    text
                },
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn apply_weekly_config(group_id: String, weekly_wallpapers: serde_json::Value) -> String {
    println!("Backend: Applying weekly config for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("{}class-wallpapers/apply-weekly", API_BASE_URL);
    
    let params = serde_json::json!({
        "group_id": group_id,
        "weekly_wallpapers": weekly_wallpapers
    });
    
    match client.post(&url).json(&params).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => {
                    println!("Backend: Apply weekly response: {}", text);
                    text
                },
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn disable_weekly_wallpaper(group_id: String) -> String {
    println!("Backend: Disabling weekly wallpaper for group_id: {}", group_id);
    let client = reqwest::Client::new();
    let url = format!("{}class-wallpapers/disable-weekly", API_BASE_URL);
    
    let params = serde_json::json!({
        "group_id": group_id
    });
    
    match client.post(&url).json(&params).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => {
                    println!("Backend: Disable weekly response: {}", text);
                    text
                },
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

const API_BASE_URL: &str = "http://47.100.126.194:5000/";

#[tauri::command]
async fn download_wallpaper(group_id: String, wallpaper_id: i32) -> String {
    let client = reqwest::Client::new();
    let url = format!("{}class-wallpapers/download", API_BASE_URL);
    let params = serde_json::json!({
        "group_id": group_id,
        "wallpaper_id": wallpaper_id
    });

    match client.post(&url).json(&params).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => text,
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn upload_wallpaper(group_id: String, image_data: Vec<u8>, mime_type: String) -> String {
    let client = reqwest::Client::new();
    let url = format!("{}class-wallpapers/upload", API_BASE_URL);
    
    // Convert bytes to base64 with data URI prefix
    let base64_data = base64::Engine::encode(&base64::engine::general_purpose::STANDARD, &image_data);
    let image_with_prefix = format!("data:{};base64,{}", mime_type, base64_data);
    
    // Generate a name based on timestamp
    let timestamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_secs();
    let name = format!("自定义壁纸_{}", timestamp);
    
    let params = serde_json::json!({
        "group_id": group_id,
        "image": image_with_prefix,
        "name": name
    });
    
    println!("Backend: Uploading wallpaper to {} for group {}", url, group_id);
    
    match client.post(&url).json(&params).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => {
                    println!("Backend: Upload response: {}", text);
                    text
                },
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn save_student_score_sheet(
    class_id: String,
    term: String,
    exam_name: String,
    scores: Vec<serde_json::Value>,
    fields: Vec<serde_json::Value>
) -> String {
    let client = reqwest::Client::new();
    let url = format!("{}student-scores/save", API_BASE_URL);
    
    let params = serde_json::json!({
        "class_id": class_id,
        "term": term,
        "exam_name": exam_name,
        "operation_mode": "replace",
        "scores": scores,
        "fields": fields
    });

    match client.post(&url).json(&params).send().await {
        Ok(res) => {
            match res.text().await {
                Ok(text) => text,
                Err(e) => format!("{{\"code\": 500, \"message\": \"Failed to read response: {}\"}}", e)
            }
        },
        Err(e) => format!("{{\"code\": 500, \"message\": \"Request failed: {}\"}}", e)
    }
}

#[tauri::command]
async fn save_course_schedule(class_id: String, term: String, days: Vec<String>, times: Vec<String>, cells: serde_json::Value, token: String) -> Result<String, String> {
    println!("Backend: Saving course schedule for class_id: {}", class_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/course-schedule/save"; 

    let response = client.post(url)
        .header("Authorization", token)
        .json(&serde_json::json!({
            "class_id": class_id,
            "term": term,
            "days": days,
            "times": times,
            "cells": cells
        }))
        .send().await.map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}



#[tauri::command]
async fn update_user_name(phone: String, name: String, id_number: String) -> Result<String, String> {
    println!("Backend: update_user_name called. Phone: {}, Name: {}", phone, name);
    let client = reqwest::Client::new();
    let url = format!("{}updateUserName", API_BASE_URL);

    let response = client.post(&url)
        .form(&serde_json::json!({
            "phone": phone,
            "name": name,
            "id_number": id_number
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn update_user_info(
    phone: String, id_number: String, name: String, avatar: String, 
    sex: String, address: String, school_name: String, 
    grade_level: String, is_administrator: String
) -> Result<String, String> {
    println!("Backend: update_user_info called. Phone: {}", phone);
    let client = reqwest::Client::new();
    let url = format!("{}updateUserInfo", API_BASE_URL);

    let response = client.post(&url)
        .form(&serde_json::json!({
            "phone": phone,
            "id_number": id_number,
            "name": name,
            "avatar": avatar,
            "sex": sex,
            "address": address,
            "school_name": school_name,
            "grade_level": grade_level,
            "is_administrator": is_administrator
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}



#[tauri::command]
async fn update_user_administrator(phone: String, id_number: String, is_administrator: String) -> Result<String, String> {
    println!("Backend: update_user_administrator called. Phone: {}, Status: {}", phone, is_administrator);
    let client = reqwest::Client::new();
    let url = format!("{}updateUserAdministrator", API_BASE_URL);

    let response = client.post(&url)
        .form(&serde_json::json!({
            "phone": phone,
            "id_number": id_number,
            "is_administrator": is_administrator
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}



#[tauri::command]
async fn get_school_by_name(name: String) -> Result<String, String> {
    println!("Backend: get_school_by_name called. Name: {}", name);
    let client = reqwest::Client::new();
    let url = format!("{}schools?name={}", API_BASE_URL, name);

    let response = client.get(&url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn get_unique_6_digit() -> Result<String, String> {
    println!("Backend: get_unique_6_digit called.");
    let client = reqwest::Client::new();
    let url = format!("{}unique6digit", API_BASE_URL);

    // Qt uses GET for this based on `m_httpHandler->get` call in line 194 of QSchoolInfoWidget.cpp
    let response = client.get(&url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn update_school_info(id: String, name: String, address: String) -> Result<String, String> {
    println!("Backend: update_school_info called. ID: {}, Name: {}", id, name);
    let client = reqwest::Client::new();
    let url = format!("{}updateSchoolInfo", API_BASE_URL);

    let response = client.post(&url)
        .form(&serde_json::json!({
            "id": id,
            "name": name,
            "address": address
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn request_server_leave_group(group_id: String, user_id: String) -> Result<String, String> {
    println!("Backend: Requesting server to leave group: {}, user: {}", group_id, user_id);
    let client = reqwest::Client::new();
    let url = format!("{}groups/leave", API_BASE_URL);

    let response = client.post(&url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "user_id": user_id
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn request_server_dismiss_group(group_id: String, user_id: String) -> Result<String, String> {
    println!("Backend: Requesting server to dismiss group: {}, user: {}", group_id, user_id);
    let client = reqwest::Client::new();
    let url = format!("{}groups/dismiss", API_BASE_URL);

    let response = client.post(&url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "user_id": user_id
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn get_classes_by_prefix(prefix: String) -> Result<String, String> {
    println!("Backend: get_classes_by_prefix called with prefix: {}", prefix);
    let client = reqwest::Client::new();
    let url = format!("{}{}", API_BASE_URL, "getClassesByPrefix");

    let response = client.post(&url)
        .json(&serde_json::json!({
            "prefix": prefix
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)

}

#[tauri::command]
async fn get_class_info(class_code: String, token: Option<String>) -> Result<String, String> {
    println!("Backend: get_class_info called. class_code: {}", class_code);
    let client = reqwest::Client::new();
    let url = format!("{}api/class/info?class_code={}", API_BASE_URL, class_code);

    let mut request = client.get(&url);
    if let Some(t) = token {
        if !t.is_empty() {
            request = request.header("Authorization", format!("Bearer {}", t));
        }
    }

    let response = request
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn update_class_avatar(class_code: String, avatar: String, token: Option<String>) -> Result<String, String> {
    println!("Backend: update_class_avatar called. class_code: {}", class_code);
    let client = reqwest::Client::new();
    let url = format!("{}classes/update-avatar", API_BASE_URL);

    let mut request = client.post(&url).json(&serde_json::json!({
        "class_code": class_code,
        "avatar": avatar
    }));

    if let Some(t) = token {
        if !t.is_empty() {
            request = request.header("Authorization", format!("Bearer {}", t));
        }
    }

    let response = request
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn update_classes(classes: Vec<serde_json::Value>) -> Result<String, String> {
    println!("Backend: update_classes called. Count: {}", classes.len());
    let client = reqwest::Client::new();
    let url = format!("{}updateClasses", API_BASE_URL);

    // Ensure payload is a JSON array
    let response = client.post(&url)
        .json(&classes)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn delete_classes(classes: Vec<serde_json::Value>) -> Result<String, String> {
    println!("Backend: delete_classes called. Count: {}", classes.len());
    let client = reqwest::Client::new();
    // Use deleteClasses endpoint
    let url = format!("{}deleteClasses", API_BASE_URL);

    let response = client.post(&url)
        .json(&classes)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}



#[tauri::command]
async fn get_list_teachers(school_id: String) -> Result<String, String> {
    println!("Backend: get_list_teachers called. SchoolID: {}", school_id);
    let client = reqwest::Client::new();
    let url = format!("{}get_list_teachers?schoolId={}", API_BASE_URL, school_id);

    let response = client.get(&url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn add_teacher(teachers: Vec<serde_json::Value>) -> Result<String, String> {
    println!("Backend: add_teacher called. Count: {}", teachers.len());
    let client = reqwest::Client::new();
    let url = format!("{}add_teacher", API_BASE_URL);

    // Endpoint name is add_teacher, but usually accepts a list or single? 
    // QMemberManager sends a JSON list.
    let response = client.post(&url)
        .json(&teachers)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn delete_teacher(phones: Vec<String>) -> Result<String, String> {
    println!("Backend: delete_teacher called. Count: {}", phones.len());
    let client = reqwest::Client::new();
    let url = format!("{}delete_teacher", API_BASE_URL);

    // QMemberManager sends a JSON list of phones to delete?
    // Or objects? `delete_teacher` usually takes list of IDs or phones.
    // Let's assume list of phones for now based on usual patterns, or objects if generic.
    // Qt: `QJsonObject json; json.insert("phone", phone); array.append(json);`
    // So it sends an array of objects: `[{"phone": "..."}]`
    
    let payload: Vec<_> = phones.into_iter().map(|p| serde_json::json!({ "phone": p })).collect();

    let response = client.post(&url)
        .json(&payload)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn search_classes(keyword: String, school_id: Option<String>) -> Result<String, String> {
    println!("Backend: search_classes called with keyword: {}, school_id: {:?}", keyword, school_id);
    let client = reqwest::Client::new();
    let mut url = format!("{}classes/search?class_code={}", API_BASE_URL, keyword);

    if let Some(sid) = school_id {
        if !sid.is_empty() {
            url.push_str(&format!("&schoolid={}", sid));
        }
    }

    let response = client.get(&url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn search_teachers(keyword: String) -> Result<String, String> {
    println!("Backend: search_teachers called with keyword: {}", keyword);
    let client = reqwest::Client::new();
    let url = format!("{}teachers/search", API_BASE_URL);
    
    // Determine if keyword looks like ID or Name (Simple logic: check if purely numeric/alphanumeric with special chars vs chinese)
    // Rust doesn't have easy "containsChinese" regex without crate, but we can pass generic params or just try one.
    // The python API likely handles query params manually.
    // Logic from SearchDialog.h: if looksLikeId -> teacher_unique_id else name.
    
    let is_id = keyword.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_');
    
    let query_param = if is_id {
        format!("teacher_unique_id={}", keyword)
    } else {
        format!("name={}", keyword)
    };
    
    let full_url = format!("{}?{}", url, query_param);

    let response = client.get(&full_url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn search_class_groups(keyword: String) -> Result<String, String> {
    println!("Backend: search_class_groups called with keyword: {}", keyword);
    let client = reqwest::Client::new();
    let url = format!("{}groups/search", API_BASE_URL);
    
    let is_id = keyword.chars().all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_');
    
    let query_param = if is_id {
        format!("group_id={}", keyword)
    } else {
        format!("group_name={}", keyword)
    };
    
    let full_url = format!("{}?{}", url, query_param);

    let response = client.get(&full_url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn join_class_group_request(group_id: String, user_id: String, user_name: String, reason: String) -> Result<String, String> {
    println!("Backend: join_class_group_request called. Group: {}, User: {}", group_id, user_id);
    let client = reqwest::Client::new();
    let url = format!("{}groups/join", API_BASE_URL); 

    let response = client.post(&url)
        .json(&serde_json::json!({
            "group_id": group_id,
            "user_id": user_id,
            "user_name": user_name,
            "reason": reason
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn join_class(teacher_unique_id: String, class_code: String) -> Result<String, String> {
    println!("Backend: join_class called. Teacher: {}, ClassCode: {}", teacher_unique_id, class_code);
    let client = reqwest::Client::new();
    let url = format!("{}teachers/classes/add", API_BASE_URL);

    let response = client.post(&url)
        .json(&serde_json::json!({
            "teacher_unique_id": teacher_unique_id,
            "class_code": class_code,
            "role": "teacher"
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

// Helper to fetch UserSig internally
async fn fetch_user_sig_internal(user_id: &str) -> Result<String, String> {
    let client = reqwest::Client::new();
    let url = format!("{}getUserSig", API_BASE_URL);
    let res = client.post(&url)
        .form(&serde_json::json!({ "user_id": user_id }))
        .send().await.map_err(|e| e.to_string())?;

    if res.status().is_success() {
        let text = res.text().await.map_err(|e| e.to_string())?;
        let json: serde_json::Value = serde_json::from_str(&text).map_err(|e| e.to_string())?;
        let sig = json["data"]["user_sig"].as_str()
            .or_else(|| json["data"]["usersig"].as_str())
            .or_else(|| json["data"]["sig"].as_str())
            .or_else(|| json["user_sig"].as_str())
            .or_else(|| json["usersig"].as_str())
            .or_else(|| json["sig"].as_str());
        
        if let Some(s) = sig {
            Ok(s.to_string())
        } else {
            Err("UserSig not found".to_string())
        }
    } else {
        Err(format!("Request failed: {}", res.status()))
    }
}

#[tauri::command]
async fn create_group_tim(owner_id: String, group_name: String, group_type: String) -> Result<String, String> {
    println!("Backend: create_group_tim called. Owner: {}, Name: {}, Type: {}", owner_id, group_name, group_type);
    
    // 1. Get UserSig
    let sig = fetch_user_sig_internal(&owner_id).await?;
    
    // 2. Construct TIM URL
    let sdk_app_id = 1600111046;
    let random = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap().as_secs();
    let url = format!("https://console.tim.qq.com/v4/group_open_http_svc/create_group?sdkappid={}&identifier={}&usersig={}&random={}&contenttype=json", 
        sdk_app_id, owner_id, sig, random);

    // 3. Construct Body
    let client = reqwest::Client::new();
    let response = client.post(&url)
        .json(&serde_json::json!({
            "Name": group_name,
            "Type": group_type, 
            "Owner_Account": owner_id,
            "GroupConfig": {
                "MaxMemberCount": 2000,
                "ApplyJoinOption": "FreeAccess"
            },
            "MemberList": [
                { 
                    "Member_Account": owner_id
                }
            ]
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;
        
    let text = response.text().await.map_err(|e| e.to_string())?;
    println!("Backend: Tencent API Response: {}", text);
    Ok(text)
}

#[tauri::command]
async fn remove_friend(teacher_unique_id: String, friend_teacher_unique_id: String) -> Result<String, String> {
    println!("Backend: remove_friend called. userId: {}, friendId: {}", teacher_unique_id, friend_teacher_unique_id);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/friends/remove";
    
    let res = client.post(url)
        .json(&serde_json::json!({
             "teacher_unique_id": teacher_unique_id,
             "friend_teacher_unique_id": friend_teacher_unique_id
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    let text = res.text().await.map_err(|e| e.to_string())?;

    if status.is_success() {
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}. Body: {}", status, text))
    }
}

#[tauri::command]
async fn leave_class(teacher_unique_id: String, class_code: String) -> Result<String, String> {
    println!("Backend: leave_class called. userId: {}, classCode: {}", teacher_unique_id, class_code);
    let client = reqwest::Client::new();
    let url = "http://47.100.126.194:5000/teachers/classes/remove";
    
    let res = client.post(url)
        .json(&serde_json::json!({
             "teacher_unique_id": teacher_unique_id,
             "class_code": class_code
        }))
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let status = res.status();
    let text = res.text().await.map_err(|e| e.to_string())?;

    if status.is_success() {
        Ok(text)
    } else {
        Err(format!("Request failed with status: {}. Body: {}", status, text))
    }
}

#[tauri::command]
async fn exit_app() {
    std::process::exit(0);
}

#[tauri::command]
async fn save_seat_arrangement(class_id: String, seats_json: String) -> Result<String, String> {
    println!("Backend: save_seat_arrangement called. ClassID: {}", class_id);
    let client = reqwest::Client::new();
    let url = format!("{}seat-arrangement/save", API_BASE_URL);

    let response = client.post(&url)
        .header("Content-Type", "application/json")
        .body(seats_json)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn get_student_scores(class_id: String) -> Result<String, String> {
    println!("Backend: get_student_scores called for class_id: {}", class_id);
    let client = reqwest::Client::new();
    let url = format!("{}student-scores?class_id={}", API_BASE_URL, class_id);
    
    let response = client.get(&url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn get_group_scores(class_id: String, term: String) -> Result<String, String> {
    println!("Backend: get_group_scores called for class_id: {}, term: {}", class_id, term);
    let client = reqwest::Client::new();
    let url = format!("{}group-scores?class_id={}&term={}", API_BASE_URL, class_id, term);
    
    let response = client.get(&url)
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
async fn update_group_settings(group_id: String, setting_name: String, value: i32, token: Option<String>) -> Result<String, String> {
    println!("Backend: update_group_settings called. group_id: {}, setting: {} = {}", group_id, setting_name, value);
    let client = reqwest::Client::new();
    let url = format!("{}groups/update-settings", API_BASE_URL);

    let mut payload = serde_json::json!({
        "group_id": group_id,
    });
    payload[&setting_name] = serde_json::json!(value);

    let mut request = client.post(&url).json(&payload);

    if let Some(t) = token {
        if !t.is_empty() {
            request = request.header("Authorization", format!("Bearer {}", t));
        }
    }

    let response = request
        .send()
        .await
        .map_err(|e| e.to_string())?;

    let text = response.text().await.map_err(|e| e.to_string())?;
    Ok(text)
}

#[tauri::command]
fn get_system_icon(path: String, size: i32) -> Result<String, String> {
    #[cfg(target_os = "windows")]
    {
        use windows::core::PCWSTR;
        use windows::Win32::UI::Shell::{SHGetFileInfoW, SHGFI_ICON, SHGFI_LARGEICON, SHGFI_SMALLICON, SHFILEINFOW};
        use windows::Win32::UI::WindowsAndMessaging::{DestroyIcon, GetIconInfo};
        use windows::Win32::Graphics::Gdi::{BITMAP, CreateCompatibleDC, SelectObject, DeleteDC, DeleteObject, GetDIBits, BITMAPINFOHEADER, DIB_RGB_COLORS, GetObjectW};
        use std::os::windows::ffi::OsStrExt;
        use base64::{Engine as _, engine::general_purpose};
        use image::{RgbaImage, ImageFormat};
        use std::io::Cursor;

        let path_u16: Vec<u16> = std::ffi::OsStr::new(&path).encode_wide().chain(Some(0)).collect();
        let mut shfi = SHFILEINFOW::default();
        
        let flags = if size > 16 {
            SHGFI_ICON | SHGFI_LARGEICON
        } else {
            SHGFI_ICON | SHGFI_SMALLICON
        };

        unsafe {
            let result = SHGetFileInfoW(
                PCWSTR(path_u16.as_ptr()),
                windows::Win32::Storage::FileSystem::FILE_FLAGS_AND_ATTRIBUTES::default(),
                Some(&mut shfi),
                std::mem::size_of::<SHFILEINFOW>() as u32,
                flags,
            );

            if result == 0 || shfi.hIcon.is_invalid() {
                return Err("Failed to get icon info".into());
            }

            let hicon = shfi.hIcon;
            let mut icon_info = windows::Win32::UI::WindowsAndMessaging::ICONINFO::default();
            if GetIconInfo(hicon, &mut icon_info).is_err() {
                let _ = DestroyIcon(hicon);
                return Err("Failed to get icon info details".into());
            }

            // Get bitmap dimensions
            let mut bmp = BITMAP::default();
            GetObjectW(icon_info.hbmColor, std::mem::size_of::<BITMAP>() as i32, Some(&mut bmp as *mut _ as *mut _));
            
            let width = bmp.bmWidth;
            let height = bmp.bmHeight;

            let hdc = CreateCompatibleDC(None);
            let mut bmi = windows::Win32::Graphics::Gdi::BITMAPINFO {
                bmiHeader: BITMAPINFOHEADER {
                    biSize: std::mem::size_of::<BITMAPINFOHEADER>() as u32,
                    biWidth: width,
                    biHeight: -height, // top-down
                    biPlanes: 1,
                    biBitCount: 32,
                    biCompression: 0, // BI_RGB
                    ..Default::default()
                },
                ..Default::default()
            };

            let mut buffer = vec![0u8; (width * height * 4) as usize];
            let old_obj = SelectObject(hdc, icon_info.hbmColor);
            
            GetDIBits(
                hdc,
                icon_info.hbmColor,
                0,
                height as u32,
                Some(buffer.as_mut_ptr() as *mut _),
                &mut bmi,
                DIB_RGB_COLORS,
            );

            SelectObject(hdc, old_obj);
            DeleteDC(hdc);
            let _ = DeleteObject(icon_info.hbmColor);
            let _ = DeleteObject(icon_info.hbmMask);
            let _ = DestroyIcon(hicon);

            // BGRA to RGBA
            for chunk in buffer.chunks_mut(4) {
                chunk.swap(0, 2);
            }

            let img = RgbaImage::from_raw(width as u32, height as u32, buffer)
                .ok_or("Failed to create image from raw buffer")?;
            
            let mut cursor = Cursor::new(Vec::new());
            img.write_to(&mut cursor, ImageFormat::Png).map_err(|e| e.to_string())?;
            
            Ok(format!("data:image/png;base64,{}", general_purpose::STANDARD.encode(cursor.into_inner())))
        }
    }
    #[cfg(not(target_os = "windows"))]
    {
        Err("Not implemented for this OS".into())
    }
}

#[tauri::command]
fn create_file_box(app: tauri::AppHandle) -> Result<serde_json::Value, String> {
    use std::time::{SystemTime, UNIX_EPOCH};
    use tauri::path::BaseDirectory;

    let base_dir = app
        .path()
        .resolve("EduDesk/Boxes", BaseDirectory::Document)
        .map_err(|e| e.to_string())?;

    std::fs::create_dir_all(&base_dir).map_err(|e| e.to_string())?;

    let ts = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map_err(|e| e.to_string())?
        .as_millis();
    let box_id = format!("box_{}", ts);
    let box_path = base_dir.join(&box_id);
    std::fs::create_dir_all(&box_path).map_err(|e| e.to_string())?;

    Ok(serde_json::json!({
        "id": box_id,
        "path": box_path.to_string_lossy().to_string(),
        "name": "新建盒子"
    }))
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
struct FileItem {
    name: String,
    path: String,
    is_dir: bool,
    size: Option<u64>,
}

#[tauri::command]
fn list_box_files(app: tauri::AppHandle, box_id: String) -> Result<Vec<FileItem>, String> {
    use tauri::path::BaseDirectory;

    let base_dir = app
        .path()
        .resolve(format!("EduDesk/Boxes/{}", box_id), BaseDirectory::Document)
        .map_err(|e| e.to_string())?;

    let mut items = Vec::new();
    let entries = std::fs::read_dir(&base_dir).map_err(|e| e.to_string())?;
    for entry in entries {
        let entry = entry.map_err(|e| e.to_string())?;
        let path = entry.path();
        let name = entry
            .file_name()
            .to_string_lossy()
            .to_string();
        let metadata = entry.metadata().map_err(|e| e.to_string())?;
        let is_dir = metadata.is_dir();
        let size = if is_dir { None } else { Some(metadata.len()) };
        items.push(FileItem {
            name,
            path: path.to_string_lossy().to_string(),
            is_dir,
            size,
        });
    }

    Ok(items)
}

#[tauri::command]
fn create_box_folder(app: tauri::AppHandle, box_id: String, folder_name: String) -> Result<(), String> {
    use tauri::path::BaseDirectory;

    let base_dir = app
        .path()
        .resolve(format!("EduDesk/Boxes/{}", box_id), BaseDirectory::Document)
        .map_err(|e| e.to_string())?;
    let target = base_dir.join(folder_name);
    std::fs::create_dir_all(&target).map_err(|e| e.to_string())?;
    Ok(())
}

#[tauri::command]
fn import_files_to_box(app: tauri::AppHandle, box_id: String, paths: Vec<String>) -> Result<(), String> {
    use std::path::{Path, PathBuf};
    use std::time::{SystemTime, UNIX_EPOCH};
    use tauri::path::BaseDirectory;

    fn unique_dest(dir: &Path, name: &str) -> PathBuf {
        let mut candidate = dir.join(name);
        if !candidate.exists() {
            return candidate;
        }
        let ts = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_millis())
            .unwrap_or(0);
        let (stem, ext) = match name.rsplit_once('.') {
            Some((s, e)) => (s.to_string(), format!(".{}", e)),
            None => (name.to_string(), "".to_string()),
        };
        candidate = dir.join(format!("{}_{}{}", stem, ts, ext));
        candidate
    }

    fn copy_dir_all(src: &Path, dst: &Path) -> std::io::Result<()> {
        std::fs::create_dir_all(dst)?;
        for entry in std::fs::read_dir(src)? {
            let entry = entry?;
            let file_type = entry.file_type()?;
            let dest_path = dst.join(entry.file_name());
            if file_type.is_dir() {
                copy_dir_all(&entry.path(), &dest_path)?;
            } else {
                std::fs::copy(entry.path(), dest_path)?;
            }
        }
        Ok(())
    }

    let base_dir = app
        .path()
        .resolve(format!("EduDesk/Boxes/{}", box_id), BaseDirectory::Document)
        .map_err(|e| e.to_string())?;
    std::fs::create_dir_all(&base_dir).map_err(|e| e.to_string())?;

    for src in paths {
        let src_path = PathBuf::from(&src);
        if !src_path.exists() {
            continue;
        }
        let name = src_path.file_name().and_then(|n| n.to_str()).unwrap_or("item");
        let dest_path = unique_dest(&base_dir, name);

        if src_path.is_dir() {
            // Try rename first, fallback to copy+remove
            if std::fs::rename(&src_path, &dest_path).is_err() {
                copy_dir_all(&src_path, &dest_path).map_err(|e| e.to_string())?;
                std::fs::remove_dir_all(&src_path).map_err(|e| e.to_string())?;
            }
        } else {
            if std::fs::rename(&src_path, &dest_path).is_err() {
                std::fs::copy(&src_path, &dest_path).map_err(|e| e.to_string())?;
                std::fs::remove_file(&src_path).map_err(|e| e.to_string())?;
            }
        }
    }

    Ok(())
}

#[tauri::command]
fn start_file_drag(paths: Vec<String>) -> Result<(), String> {
    #[cfg(target_os = "windows")]
    {
        use std::ffi::OsStr;
        use std::os::windows::ffi::OsStrExt;
        use std::path::Path;
        use windows::core::{PCWSTR, HRESULT};
        use windows::Win32::Foundation::BOOL;
        use windows::Win32::System::Com::{CoInitializeEx, CoTaskMemFree, CoUninitialize, COINIT_APARTMENTTHREADED};
        use windows::Win32::System::Ole::{DoDragDrop, DROPEFFECT, DROPEFFECT_COPY, DROPEFFECT_MOVE};
        use windows::Win32::System::Com::IDataObject;
        use windows::Win32::System::SystemServices::MODIFIERKEYS_FLAGS;
        use windows::Win32::UI::Shell::{ILFindLastID, SHCreateDataObject, SHParseDisplayName};
        use windows::Win32::UI::Shell::Common::ITEMIDLIST;
        use windows::Win32::System::Ole::IDropSource;

        #[windows::core::implement(IDropSource)]
        struct DropSource;
        #[allow(non_snake_case)]
        impl windows::Win32::System::Ole::IDropSource_Impl for DropSource {
            fn QueryContinueDrag(&self, fEscapePressed: BOOL, grfKeyState: MODIFIERKEYS_FLAGS) -> HRESULT {
                const DRAGDROP_S_CANCEL: HRESULT = HRESULT(0x00040101);
                const DRAGDROP_S_DROP: HRESULT = HRESULT(0x00040100);
                const MK_LBUTTON: u32 = 0x0001;
                if fEscapePressed.as_bool() {
                    return DRAGDROP_S_CANCEL;
                }
                if (grfKeyState.0 & MK_LBUTTON) == 0 {
                    return DRAGDROP_S_DROP;
                }
                HRESULT(0)
            }
            fn GiveFeedback(&self, _dwEffect: DROPEFFECT) -> HRESULT {
                const DRAGDROP_S_USEDEFAULTCURSORS: HRESULT = HRESULT(0x00040102);
                DRAGDROP_S_USEDEFAULTCURSORS
            }
        }

        if paths.is_empty() {
            return Err("未提供拖拽路径".into());
        }

        let first_path = Path::new(&paths[0]);
        let parent = first_path.parent().ok_or("无法获取父目录")?;
        for p in &paths[1..] {
            let cur = Path::new(p);
            let cur_parent = cur.parent().ok_or("无法获取父目录")?;
            if cur_parent != parent {
                return Err("暂不支持跨目录多选拖拽".into());
            }
        }

        unsafe {
            CoInitializeEx(None, COINIT_APARTMENTTHREADED).map_err(|e| e.message().to_string())?;
        }

        let parent_wide: Vec<u16> = OsStr::new(parent.as_os_str()).encode_wide().chain(Some(0)).collect();
        let mut pidl_folder: *mut ITEMIDLIST = std::ptr::null_mut();
        let mut pidl_full_list: Vec<*mut ITEMIDLIST> = Vec::new();
        let mut child_list: Vec<*const ITEMIDLIST> = Vec::new();

        unsafe {
            SHParseDisplayName(PCWSTR(parent_wide.as_ptr()), None, &mut pidl_folder, 0, None)
                .map_err(|e| e.message().to_string())?;
            for full_path in &paths {
                let full_wide: Vec<u16> =
                    OsStr::new(full_path.as_str()).encode_wide().chain(Some(0)).collect();
                let mut pidl_full: *mut ITEMIDLIST = std::ptr::null_mut();
                SHParseDisplayName(PCWSTR(full_wide.as_ptr()), None, &mut pidl_full, 0, None)
                    .map_err(|e| e.message().to_string())?;
                let child = ILFindLastID(pidl_full) as *const ITEMIDLIST;
                pidl_full_list.push(pidl_full);
                child_list.push(child);
            }

            let data_obj: IDataObject = SHCreateDataObject(
                Some(pidl_folder as *const ITEMIDLIST),
                Some(&child_list),
                None,
            )
            .map_err(|e| e.message().to_string())?;

            let drop_source: IDropSource = DropSource.into();
            let mut effect = DROPEFFECT_COPY | DROPEFFECT_MOVE;
            let _ = DoDragDrop(&data_obj, &drop_source, effect, &mut effect);

            CoTaskMemFree(Some(pidl_folder as _));
            for pidl in pidl_full_list {
                CoTaskMemFree(Some(pidl as _));
            }
            CoUninitialize();
        }

        return Ok(());
    }

    #[cfg(not(target_os = "windows"))]
    {
        let _ = paths;
        Err("start_file_drag 仅支持 Windows".into())
    }
}

// Global state for ffmpeg process
use std::sync::Mutex;
use std::process::{Command as StdCommand, Child};

struct StreamState {
    process: Mutex<Option<Child>>,
}

// Safer to wrap in a struct managed by Tauri, but for quick implementation we can use a lazy_static or direct struct.
// However, fitting into the 'run' function with `manage` is better pattern.
// Let's implement the commands first.

#[tauri::command]
async fn start_stream(app: tauri::AppHandle, pull_url: String) -> Result<String, String> {
    println!("Backend: start_stream called. Pull URL: {}", pull_url);

    // Stop existing stream if any
    stop_stream(app.clone()).await?;

    // Find camera/mic devices (Simplification: using default dshow devices or specific ones if known)
    // For a robust app, we should list devices and let user choose, or pick first available.
    // Command: ffmpeg -f dshow -i video="Integrated Camera":audio="Microphone Array" ...
    // NOTE: This relies on ffmpeg being in PATH or bundled.
    
    // Hardcoded path provided by user
    const FFMPEG_PATH: &str = r"D:\Agreement\ClassAssistantClient\ffmpeg-n6.0.1-win64-gpl-shared-6.0\bin\ffmpeg.exe";
    
    println!("Backend: Listing DirectShow devices via ffmpeg");
    let list_cmd = StdCommand::new(FFMPEG_PATH)
        .args(&["-list_devices", "true", "-f", "dshow", "-i", "dummy"])
        .output();
        
    let mut video_device = String::new();
    let mut audio_device = String::new();

    if let Ok(output) = list_cmd {
        println!(
            "Backend: ffmpeg device list exit: {} (stdout {} bytes, stderr {} bytes)",
            output.status,
            output.stdout.len(),
            output.stderr.len()
        );
        let stderr = String::from_utf8_lossy(&output.stderr);
        println!("Backend: ffmpeg device list stderr begin");
        for line in stderr.lines() {
            println!("Backend: ffmpeg dshow: {}", line);
        }
        println!("Backend: ffmpeg device list stderr end");
        // Parse stderr for "DirectShow video devices" and "DirectShow audio devices"
        // Minimal parser logic...
        let mut in_video = false;
        let mut in_audio = false;
        for line in stderr.lines() {
            if line.contains("DirectShow video devices") {
                in_video = true;
                in_audio = false;
                continue;
            }
            if line.contains("DirectShow audio devices") {
                in_video = false;
                in_audio = true;
                continue;
            }
            
            // Typical line: [dshow @ ...]  "Camera Name"
            if (in_video || in_audio) && line.contains("\"") {
                if let Some(start) = line.find('"') {
                    if let Some(end) = line[start+1..].find('"') {
                        let name = &line[start+1..start+1+end];
                        if in_video && video_device.is_empty() {
                            video_device = name.to_string();
                            println!("Backend: Selected video device: {}", video_device);
                        } else if in_audio && audio_device.is_empty() {
                            audio_device = name.to_string();
                            println!("Backend: Selected audio device: {}", audio_device);
                        }
                    }
                }
            }

            // Fallback: some ffmpeg builds don't print the "DirectShow video/audio devices" headers.
            // Detect by "(video)" / "(audio)" suffix on the same line.
            if line.contains("\"") && (line.contains("(video)") || line.contains("(audio)")) {
                if let Some(start) = line.find('"') {
                    if let Some(end) = line[start + 1..].find('"') {
                        let name = &line[start + 1..start + 1 + end];
                        if line.contains("(video)") && video_device.is_empty() {
                            video_device = name.to_string();
                            println!("Backend: Selected video device (fallback): {}", video_device);
                        } else if line.contains("(audio)") && audio_device.is_empty() {
                            audio_device = name.to_string();
                            println!("Backend: Selected audio device (fallback): {}", audio_device);
                        }
                    }
                }
            }
        }
    }
    
    if video_device.is_empty() {
        return Err("No video device found".to_string());
    }
    if audio_device.is_empty() {
        // Fallback or just ignore audio? Let's try to proceed without audio if not found?
        // Protocol usually implies AV.
        println!("Backend: Warning - No audio device found.");
    }

    println!("Backend: Using Video='{}', Audio='{}'", video_device, audio_device);

    let args = vec![
        "-f".to_string(), "dshow".to_string(),
        "-rtbufsize".to_string(), "100M".to_string(),
        "-i".to_string(),
        if audio_device.is_empty() {
            format!("video={}", video_device)
        } else {
            format!("video={}:audio={}", video_device, audio_device)
        },
        "-c:v".to_string(), "libx264".to_string(),
        "-preset".to_string(), "ultrafast".to_string(),
        "-tune".to_string(), "zerolatency".to_string(),
        "-c:a".to_string(), "aac".to_string(),
        "-b:a".to_string(), "128k".to_string(),
        "-ar".to_string(), "44100".to_string(),
        "-r".to_string(), "25".to_string(),
        "-f".to_string(), "mpegts".to_string(),
        pull_url // The target URL, e.g. srt://...
    ];
    println!("Backend: ffmpeg args: {:?}", args);

    // Spawn the process
    let child = StdCommand::new(FFMPEG_PATH)
        .args(&args)
        // .stdout(std::process::Stdio::null())
        // .stderr(std::process::Stdio::null()) // Maybe keep stderr for debugging?
        .spawn()
        .map_err(|e| format!("Failed to start ffmpeg: {}", e))?;
    println!("Backend: ffmpeg started, pid={}", child.id());

    let state = app.state::<StreamState>();
    *state.process.lock().unwrap() = Some(child);

    Ok("Stream started".to_string())
}

#[tauri::command]
async fn stop_stream(app: tauri::AppHandle) -> Result<String, String> {
    println!("Backend: stop_stream called");
    let state = app.state::<StreamState>();
    let mut process_guard = state.process.lock().unwrap();
    
    if let Some(mut child) = process_guard.take() {
        let _ = child.kill(); // Force kill
        let _ = child.wait(); // Prevent zombie
        Ok("Stream stopped".to_string())
    } else {
        Ok("No active stream".to_string())
    }
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
    .manage(StreamState { process: Mutex::new(None) })
    .plugin(tauri_plugin_opener::init())
    .plugin(tauri_plugin_fs::init())
    .plugin(tauri_plugin_shell::init())
        .on_window_event(|window, event| {
            if let tauri::WindowEvent::CloseRequested { .. } = event {
                // Ensure any active stream is stopped before window closes.
                let state = window.app_handle().state::<StreamState>();
                let mut process_guard = state.process.lock().unwrap();
                if let Some(mut child) = process_guard.take() {
                    println!("Backend: window closing, stopping active stream");
                    let _ = child.kill();
                    let _ = child.wait();
                }
            }
        })
        .invoke_handler(tauri::generate_handler![
            greet, 
            login,
            class_login,
            send_verification_code,
            register_account,
            reset_password,
            resize_window,
            get_course_schedule,
            save_course_schedule,
            get_user_friends,
            get_teacher_classes,
            get_user_info,
            get_user_sig, 
            open_class_window, 
            open_chat_window, 
            open_file_box_window,
            get_system_icon,
            list_box_files,
            import_files_to_box,
            get_group_members,
            fetch_seat_map,
            save_seat_map,
            save_teach_subjects,
            toggle_group_intercom,
            fetch_temp_room,
            open_intercom_window,
            fetch_duty_roster,
            save_duty_roster,
            fetch_class_wallpapers,
            fetch_wallpaper_library,
            set_class_wallpaper,
            download_wallpaper,
            upload_wallpaper,
            fetch_weekly_config,
            apply_weekly_config,
            disable_weekly_wallpaper,
            save_student_score_sheet,
            get_classes_by_prefix,
            get_class_info,
            search_classes,
            search_teachers,
            search_class_groups,
            join_class_group_request,
            create_group_tim,
            request_server_leave_group,
            request_server_dismiss_group,

            join_class,
            remove_friend,
            leave_class,
            update_user_name,
            update_user_info,
            update_user_administrator,
            get_school_by_name,
            get_unique_6_digit,
            update_school_info,
            update_classes,
            delete_classes,
            update_class_avatar,
            get_list_teachers,
            add_teacher,
            delete_teacher,
            update_class_avatar,
            update_group_settings,
            create_file_box,
            create_box_folder,
            save_seat_arrangement,
            get_student_scores,
            get_group_scores,
            exit_app,
            start_file_drag,
            start_stream,
            stop_stream
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
