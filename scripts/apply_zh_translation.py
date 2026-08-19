#!/usr/bin/env python3
"""Reapply the fork's Simplified Chinese UI after source updates.

The translation is intentionally deterministic and dependency-free so the
GitHub Actions job can keep the UI localized without an external service.
"""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


TRANSLATIONS = {
    "app/app/src/main/AndroidManifest.xml": [
        ('android:label="Qualcomm Band Menu"', 'android:label="@string/app_name"'),
    ],
    "app/app/src/main/res/values/strings.xml": [
        ("<string name=\"app_name\">Qualcomm Band Menu</string>",
         "<string name=\"app_name\">高通频段管理</string>"),
    ],
    "app/app/src/main/java/dev/qcom/bandmenu/ui/MainScreen.kt": [
        ('contentDescription = "Bands"', 'contentDescription = "频段"'),
        ('Text("Bands"', 'Text("频段"'),
        ('contentDescription = "Cells"', 'contentDescription = "小区"'),
        ('Text("Cells"', 'Text("小区"'),
        ('contentDescription = "Info"', 'contentDescription = "关于"'),
        ('Text("Info"', 'Text("关于"'),
        ('title = "Root Access Required"', 'title = "需要 Root 权限"'),
        (
            'summary = "This app requires root access to communicate with the Qualcomm modem. '
            'Please grant root access and retry."',
            'summary = "此应用需要 Root 权限才能与高通基带通信，请授予 Root 权限后重试。"',
        ),
        ('text = "Retry"', 'text = "重试"'),
        ('text = "OK"', 'text = "确定"'),
    ],
    "app/app/src/main/java/dev/qcom/bandmenu/ui/InfoScreen.kt": [
        ('SmallTitle("About")', 'SmallTitle("关于")'),
        ('"Qualcomm Band Menu"', '"高通频段管理"'),
        (
            '"Version: ${BuildConfig.VERSION_NAME} (${BuildConfig.VERSION_CODE})"',
            '"版本：${BuildConfig.VERSION_NAME}（${BuildConfig.VERSION_CODE}）"',
        ),
        ('"Package: ${BuildConfig.APPLICATION_ID}"', '"包名：${BuildConfig.APPLICATION_ID}"'),
        ('SmallTitle("Credits")', 'SmallTitle("致谢")'),
        ('SmallTitle("Powered by")', 'SmallTitle("技术支持")'),
        ('"miuix UI framework\\nlibsu\\nAndroidLiquidGlass"',
         '"miuix UI 框架\\nlibsu\\nAndroidLiquidGlass"'),
        ('text = "Debug logging"', 'text = "调试日志"'),
        ('contentDescription = "Menu"', 'contentDescription = "菜单"'),
    ],
    "app/app/src/main/java/dev/qcom/bandmenu/ui/BandLockScreen.kt": [
        ('SmallTopAppBar(title = "Bands"', 'SmallTopAppBar(title = "频段"'),
        ('text = "Reset"', 'text = "重置"'),
        ('Text("Apply")', 'Text("应用")'),
        ('SmallTitle("RAT lock")', 'SmallTitle("网络制式锁定")'),
        ('"AUTO (All RATs)"', '"自动（全部制式）"'),
        ('"None"', '"无"'),
        ('title = "RAT lock"', 'title = "网络制式锁定"'),
        ('SmallTitle("NR mode")', 'SmallTitle("NR 模式")'),
        ('SmallTitle("Band lock")', 'SmallTitle("频段锁定")'),
        ('"All bands (all RATs)"', '"全部频段（所有制式）"'),
        ('"All 5G bands (NSA+SA)"', '"全部 5G 频段（NSA+SA）"'),
        ('"All NR-SA bands"', '"全部 NR-SA 频段"'),
        ('"All NR-NSA bands"', '"全部 NR-NSA 频段"'),
        ('"All LTE bands"', '"全部 LTE 频段"'),
        ('"All WCDMA bands"', '"全部 WCDMA 频段"'),
        ('"All GSM bands"', '"全部 GSM 频段"'),
        ('"All NR bands"', '"全部 NR 频段"'),
        ('title = "Quick selections"', 'title = "快速选择"'),
        ('summary = "Toggle band groups"', 'summary = "切换频段组"'),
    ],
    "app/app/src/main/java/dev/qcom/bandmenu/ui/CellLockScreen.kt": [
        ('title = "Cell-Lock"', 'title = "小区锁定"'),
        ('text = "Clear ALL"', 'text = "清除全部"'),
        ('text = "Clear 5G"', 'text = "清除 5G"'),
        ('"No 5G cell lock active"', '"当前没有启用 5G 小区锁定"'),
        ('text = "Clear 4G"', 'text = "清除 4G"'),
        ('text = "Clear PLMN"', 'text = "清除 PLMN"'),
        ('text = "Enable Experimental"', 'text = "启用实验功能"'),
        ('contentDescription = "Menu"', 'contentDescription = "菜单"'),
        (
            'LabelOverride("Success locking ${lockResult.type} \\u2713"',
            'LabelOverride("已锁定 ${lockResult.type} \\u2713"',
        ),
        ('"Rejected by modem"', '"基带拒绝了请求"'),
        ('1 to "NR-ARFCN PCI SCS Band"', '1 to "NR-ARFCN PCI SCS 频段"'),
        ('2 to "NR-ARFCN SCS Band PCI1 PCI2..."',
         '2 to "NR-ARFCN SCS 频段 PCI1 PCI2..."'),
        ('3 to "\\"ID bits 22-32\\" gNB1 gNB2..."',
         '3 to "\\"ID 位数 22-32\\" gNB1 gNB2..."'),
        ('5 to "MCC MNC (e.g. 244 01)"', '5 to "MCC MNC（例如 244 01）"'),
        (
            '0 to "NR-ARFCN lock: Put NR-ARFCN and SCS (in kHz)"',
            '0 to "NR-ARFCN 锁定：输入 NR-ARFCN 和 SCS（单位：kHz）"',
        ),
        (
            '1 to "PCI lock: Put NR-ARFCN PCI SCS and Band"',
            '1 to "PCI 锁定：输入 NR-ARFCN、PCI、SCS 和频段"',
        ),
        (
            '2 to "MultiPCI lock: Put NR-ARFCN SCS Band and PCI list"',
            '2 to "多 PCI 锁定：输入 NR-ARFCN、SCS、频段和 PCI 列表"',
        ),
        (
            '3 to "gNB lock: Put ID bits (22-32) and gNB IDs"',
            '3 to "gNB 锁定：输入 ID 位数（22-32）和 gNB ID"',
        ),
        (
            '4 to "PCI lock: Put EARFCN and PCI"',
            '4 to "PCI 锁定：输入 EARFCN 和 PCI"',
        ),
        (
            '5 to "PLMN lock: Put MCC and MNC (e.g. 244 01)"',
            '5 to "PLMN 锁定：输入 MCC 和 MNC（例如 244 01）"',
        ),
        ('SmallTitle("PLMN lock")', 'SmallTitle("PLMN 锁定")'),
    ],
    "app/app/src/main/java/dev/qcom/bandmenu/MainActivity.kt": [
        ('"Daemon connection restored"', '"守护进程连接已恢复"'),
        ('"Daemon connection lost — retrying..."', '"守护进程连接已断开，正在重试……"'),
        ('"Daemon Launch Failed"', '"守护进程启动失败"'),
        ('"Initialization Failed"', '"初始化失败"'),
        (
            '"Failed to select SIM 1: ${simSet1Parsed.error?.message ?: "unknown"}"',
            '"选择 SIM 1 失败：${simSet1Parsed.error?.localizedMessage ?: "未知错误"}"',
        ),
        (
            '"Failed to query SIM 1: ${sim1Parsed.error?.message ?: "unknown"}"',
            '"读取 SIM 1 状态失败：${sim1Parsed.error?.localizedMessage ?: "未知错误"}"',
        ),
        (
            '"Failed to select SIM 2: ${sim2Parsed.error?.message ?: "unknown"}"',
            '"选择 SIM 2 失败：${sim2Parsed.error?.localizedMessage ?: "未知错误"}"',
        ),
        ('"Failed to query modem state: ${e.message}"',
         '"读取基带状态失败：${e.message}"'),
        ('"Failed to select SIM $sim"', '"选择 SIM $sim 失败"'),
        ('"Need: arfcn scs_khz"', '"需要：arfcn scs_khz"'),
        ('"Need: arfcn pci scs_khz band"', '"需要：arfcn pci scs_khz band"'),
        ('"Need: arfcn scs_khz band pci..."', '"需要：arfcn scs_khz band pci..."'),
        ('"Need: id_bits gnb..."', '"需要：id_bits gnb..."'),
        ('"Need: earfcn pci"', '"需要：earfcn pci"'),
        ('"Need: mcc mnc"', '"需要：mcc mnc"'),
        ('"MCC and MNC must be 0-999"', '"MCC 和 MNC 必须为 0-999"'),
        ('"Unknown field"', '"未知字段"'),
        ('"Rejected by modem"', '"基带拒绝了请求"'),
        ('"Apply failed: ${e.message}"', '"应用失败：${e.message}"'),
        ('2 -> "MultiPCI"', '2 -> "多 PCI"'),
        ('else -> "Unknown"', 'else -> "未知"'),
        ('"Clear all failed: ${e.message}"', '"清除全部失败：${e.message}"'),
        ('"Cleared all cell locks for SIM $sim"', '"已清除 SIM $sim 的全部小区锁定"'),
        ('"Clear 5G failed (connection error)"', '"清除 5G 失败（连接错误）"'),
        ('"Cleared 5G cell lock for SIM $sim"', '"已清除 SIM $sim 的 5G 小区锁定"'),
        ('"Clear 4G failed: ${e.message}"', '"清除 4G 失败：${e.message}"'),
        ('"Cleared 4G cell lock for SIM $sim"', '"已清除 SIM $sim 的 4G 小区锁定"'),
        ('"Clear PLMN failed: ${e.message}"', '"清除 PLMN 失败：${e.message}"'),
        ('"Cleared PLMN lock for SIM $sim"', '"已清除 SIM $sim 的 PLMN 锁定"'),
        ('"Refresh failed"', '"刷新失败"'),
        ('"Refresh failed: ${e.message}"', '"刷新失败：${e.message}"'),
        ('"Settings applied for SIM $sim"', '"SIM $sim 设置已应用"'),
        ('"Reset failed: ${e.message}"', '"重置失败：${e.message}"'),
        ('"Reset to hardware defaults for SIM $sim"', '"SIM $sim 已重置为硬件默认值"'),
        (".error?.message", ".error?.localizedMessage"),
        ("firstError.message", "firstError.localizedMessage"),
    ],
}


ERROR_TRANSLATION_BLOCK = """val DaemonError.localizedMessage: String
    get() = when (message) {
        "Unknown error" -> "未知错误"
        "NAS discovery failed." -> "NAS 服务发现失败"
        "NAS socket failed." -> "NAS 套接字创建失败"
        "SIM bind failed." -> "SIM 绑定失败"
        "State query failed." -> "读取状态失败"
        "Hardware-band service discovery failed." -> "硬件频段服务发现失败"
        "Hardware-band query failed." -> "查询硬件频段失败"
        "LTE cell-lock query failed: no reply." -> "查询 LTE 小区锁定失败：无响应"
        "LTE cell-lock query failed." -> "查询 LTE 小区锁定失败"
        "NR cell-lock query failed: no reply." -> "查询 NR 小区锁定失败：无响应"
        "NR cell-lock query failed." -> "查询 NR 小区锁定失败"
        "Command failed: no reply." -> "命令失败：无响应"
        "Command failed: malformed reply (no result TLV)." ->
            "命令失败：响应格式错误（缺少结果 TLV）"
        "Command rejected by modem." -> "基带拒绝了命令"
        "No hardware-supported LTE bands were reported." ->
            "未报告硬件支持的 LTE 频段"
        "No hardware-supported NR bands were reported." ->
            "未报告硬件支持的 NR 频段"
        "LTE command not sent." -> "LTE 命令未发送"
        "Invalid LTE list." -> "LTE 频段列表无效"
        "NR command not sent." -> "NR 命令未发送"
        "Invalid NR list." -> "NR 频段列表无效"
        "GSM command not sent." -> "GSM 命令未发送"
        "Invalid GSM list." -> "GSM 频段列表无效"
        "WCDMA command not sent." -> "WCDMA 命令未发送"
        "Invalid WCDMA list." -> "WCDMA 频段列表无效"
        "Reset failed: no hardware LTE bands reported." ->
            "重置失败：未报告硬件支持的 LTE 频段"
        "Refresh failed: could not reconnect." -> "刷新失败：无法重新连接"
        "SIM switch failed." -> "SIM 切换失败"
        "Unknown command." -> "未知命令"
        else -> message
    }
"""


def apply_replacements(path: Path, replacements: list[tuple[str, str]]) -> bool:
    original = path.read_text(encoding="utf-8")
    updated = original
    for old, new in replacements:
        updated = updated.replace(old, new)
    if updated == original:
        return False
    path.write_text(updated, encoding="utf-8")
    return True


def ensure_error_translation(path: Path) -> bool:
    content = path.read_text(encoding="utf-8")
    if "val DaemonError.localizedMessage: String" in content:
        return False
    marker = "\nobject JsonRequestBuilder {"
    if marker not in content:
        raise RuntimeError(f"Cannot locate insertion point in {path}")
    content = content.replace(
        marker,
        f"\n\n{ERROR_TRANSLATION_BLOCK}{marker}",
        1,
    )
    path.write_text(content, encoding="utf-8")
    return True


def main() -> None:
    changed = []
    for relative_path, replacements in TRANSLATIONS.items():
        path = ROOT / relative_path
        if apply_replacements(path, replacements):
            changed.append(relative_path)

    band_manager = ROOT / "app/app/src/main/java/dev/qcom/bandmenu/BandManager.kt"
    if ensure_error_translation(band_manager):
        changed.append(str(band_manager.relative_to(ROOT)))

    if changed:
        print("Updated Chinese translation in:")
        for path in changed:
            print(f"- {path}")
    else:
        print("Chinese translation is already up to date.")


if __name__ == "__main__":
    main()
