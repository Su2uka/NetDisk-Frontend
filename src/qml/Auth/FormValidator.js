// FormValidator.js — LoginPanel 和 RegisterPanel 共用的表单验证逻辑

/**
 * 验证邮箱地址
 * @param {string} email - 待验证的邮箱
 * @returns {{ ok: bool, msg: string }}
 */
function validateEmail(email) {
    email = email.trim()
    if (email === "") {
        return { ok: false, msg: "请输入邮箱账号" }
    }
    var pattern = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/
    if (!pattern.test(email)) {
        return { ok: false, msg: "请输入有效的邮箱地址" }
    }
    return { ok: true, msg: "" }
}

/**
 * 验证密码
 * @param {string} password - 待验证的密码
 * @returns {{ ok: bool, msg: string }}
 */
function validatePassword(password) {
    if (password === "") {
        return { ok: false, msg: "请输入密码" }
    }
    if (password.length < 6) {
        return { ok: false, msg: "密码长度不能小于6位" }
    }
    return { ok: true, msg: "" }
}

/**
 * 验证确认密码
 * @param {string} password - 原始密码
 * @param {string} confirm  - 确认密码
 * @returns {{ ok: bool, msg: string }}
 */
function validateConfirmPassword(password, confirm) {
    if (confirm === "") {
        return { ok: false, msg: "请再次输入密码" }
    }
    if (password !== confirm) {
        return { ok: false, msg: "两次输入的密码不一致" }
    }
    return { ok: true, msg: "" }
}
