#include "DataQualityChecker.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstring>

DataQualityChecker::DataQualityChecker() : totalEvents(0), currentEvent(0) {
}

DataQualityChecker::~DataQualityChecker() {
}

void DataQualityChecker::LogError(const std::string& message) {
    std::ostringstream os;
    os << "Event " << currentEvent << ": " << message;
    errorLog.push_back(os.str());
    std::cout << os.str() << std::endl;
}

bool DataQualityChecker::CheckEventMarkers(const std::vector<int>& buffer_v) {
    // イベントバッグのStart/Endマーカーをチェック
    if (buffer_v.size() < 4) {
        LogError("Event bag too small");
        totalErrors.sizeErrors++;
        errorsByEvent[currentEvent].sizeErrors++;
        return false;
    }
    
    // Startマーカーをチェック (0xfb 0xee 0xfb 0xee)
    if (buffer_v[0] != 0xfb || buffer_v[1] != 0xee || 
        buffer_v[2] != 0xfb || buffer_v[3] != 0xee) {
        LogError("Invalid event start marker");
        totalErrors.markerErrors++;
        errorsByEvent[currentEvent].markerErrors++;
        return false;
    }
    
    // Endマーカーをチェック (0xfe 0xdd 0xfe 0xdd)
    bool found_end = false;
    int size = buffer_v.size();
    
    if (size >= 8) {
        if (buffer_v[size-4] == 0xfe && buffer_v[size-3] == 0xdd && 
            buffer_v[size-2] == 0xfe && buffer_v[size-1] == 0xdd) {
            found_end = true;
        }
    }
    
    if (!found_end) {
        LogError("Event end marker not found or invalid");
        totalErrors.markerErrors++;
        errorsByEvent[currentEvent].markerErrors++;
        // エンドマーカーがなくてもfalseを返さない
    }
    
    return true;
}

bool DataQualityChecker::CheckSPIROCMarkers(const std::vector<int>& buffer_v) {
    // SPIROCバッグのStart/Endマーカーをチェック
    if (buffer_v.size() < 8) {
        LogError("SPIROC bag too small");
        totalErrors.sizeErrors++;
        errorsByEvent[currentEvent].sizeErrors++;
        return false;
    }
    
    // Startマーカーをチェック (0xfa 0x5a 0xfa 0x5a)
    if (buffer_v[0] != 0xfa || buffer_v[1] != 0x5a || 
        buffer_v[2] != 0xfa || buffer_v[3] != 0x5a) {
        LogError("Invalid SPIROC start marker");
        totalErrors.markerErrors++;
        errorsByEvent[currentEvent].markerErrors++;
        return false;
    }
    
    // サイズチェック
    int size = buffer_v.size();
    if (size < 4) { // 最低でもStartマーカー分
        LogError("SPIROC bag size too small");
        totalErrors.sizeErrors++;
        errorsByEvent[currentEvent].sizeErrors++;
        return false;
    }
    
    // Endマーカーが含まれているか確認 (0xfe 0xee 0xfe 0xee)
    bool found_end = false;
    for (int i = 4; i <= size - 4; i++) {
        if (buffer_v[i] == 0xfe && buffer_v[i+1] == 0xee && 
            buffer_v[i+2] == 0xfe && buffer_v[i+3] == 0xee) {
            found_end = true;
            break;
        }
    }
    
    if (!found_end) {
        LogError("SPIROC end marker not found");
        totalErrors.markerErrors++;
        errorsByEvent[currentEvent].markerErrors++;
        return false;
    }
    
    return true;
}

bool DataQualityChecker::CheckLayerID(int layer_id) {
    // Layer IDが有効範囲(0-39)内かチェック
    if (layer_id < 0 || layer_id > 39) {
        std::ostringstream os;
        os << "Invalid layer ID: " << layer_id;
        LogError(os.str());
        totalErrors.layerIDErrors++;
        errorsByEvent[currentEvent].layerIDErrors++;
        return false;
    }
    return true;
}

bool DataQualityChecker::CheckChipID(int chip_id) {
    // Chip IDが有効範囲(1-9)内かチェック
    if (chip_id < 1 || chip_id > 9) {
        std::ostringstream os;
        os << "Invalid chip ID: " << chip_id;
        LogError(os.str());
        totalErrors.chipIDErrors++;
        errorsByEvent[currentEvent].chipIDErrors++;
        return false;
    }
    return true;
}

bool DataQualityChecker::CheckSPIROCSize(const std::vector<int>& buffer_v) {
    // SPIROCバッグのサイズチェック
    int size = buffer_v.size();
    
    // 最小サイズチェック: 74バイト以上
    // if (size < 74) {
    //     std::ostringstream os;
    //     os << "SPIROC bag size too small: " << size << " (minimum 74 required)";
    //     LogError(os.str());
    //     totalErrors.sizeErrors++;
    //     errorsByEvent[currentEvent].sizeErrors++;
    //     return false;
    // }
    
    // 偶数サイズチェック
    if (size % 2 != 0) {
        std::ostringstream os;
        os << "SPIROC bag size not even: " << size;
        LogError(os.str());
        totalErrors.sizeErrors++;
        errorsByEvent[currentEvent].sizeErrors++;
        return false;
    }
    
    return true;
}

bool DataQualityChecker::CheckChipBufferSize(const std::vector<int>& chip_v) {
    // チップバッファのサイズチェック: (n×73 + 1)形式
    int size = chip_v.size();
    
    if ((size - 1) % 73 != 0) {
        std::ostringstream os;
        os << "Invalid chip buffer size: " << size << " (should be n×73 + 1)";
        LogError(os.str());
        totalErrors.chipBufferErrors++;
        errorsByEvent[currentEvent].chipBufferErrors++;
        return false;
    }
    
    return true;
}

bool DataQualityChecker::CheckTriggerIDConsistency(int triggerID, int prev_triggerID) {
    // 同一イベント内でのトリガーID一貫性チェック
    if (prev_triggerID != -1 && triggerID != prev_triggerID) {
        std::ostringstream os;
        os << "Trigger ID mismatch: current=" << triggerID << ", previous=" << prev_triggerID;
        LogError(os.str());
        totalErrors.triggerIDErrors++;
        errorsByEvent[currentEvent].triggerIDErrors++;
        return false;
    }
    return true;
}

bool DataQualityChecker::CheckEventBag(const std::vector<int>& buffer_v) {
    bool isValid = true;
    
    // 基本的なマーカーチェック
    if (!CheckEventMarkers(buffer_v)) {
        isValid = false;
    }
    
    // Cherenkov Counterの検証
    int size = buffer_v.size();
    if (size >= 8) {
        long cherenkov_counter = buffer_v[size-8]*0x1000000 + buffer_v[size-7]*0x10000 + 
                                 buffer_v[size-6]*0x100 + buffer_v[size-5];
        
        // Cherenkov Counterが有効な範囲内かチェック
        // 上位2ビットはチェレンコフ検出器のシグナル、残りはタイムスタンプ
        long timestamp = cherenkov_counter & 0x3FFFFFFF;
        if (timestamp == 0) { // タイムスタンプが0の場合、異常の可能性
            LogError("Suspicious Cherenkov counter timestamp: 0");
        }
    }
    
    if (!isValid) {
        totalErrors.eventBagErrors++;
        errorsByEvent[currentEvent].eventBagErrors++;
    }
    
    return isValid;
}

bool DataQualityChecker::CheckSPIROCBag(const std::vector<int>& buffer_v) {
    bool isValid = true;
    
    // SPIROCバッグのマーカーとサイズチェック
    if (!CheckSPIROCMarkers(buffer_v)) {
        isValid = false;
    }
    
    if (!CheckSPIROCSize(buffer_v)) {
        isValid = false;
    }
    
    // Layer FF markerチェック
    int endMarkerPos = -1;
    for (int i = 4; i <= buffer_v.size() - 4; i++) {
        if (buffer_v[i] == 0xfe && buffer_v[i+1] == 0xee && 
            buffer_v[i+2] == 0xfe && buffer_v[i+3] == 0xee) {
            endMarkerPos = i;
            break;
        }
    }
    
    if (endMarkerPos >= 0 && endMarkerPos + 5 < buffer_v.size()) {
        // Layer FF markerチェック
        if (buffer_v[endMarkerPos + 4] != 0xff) {
            LogError("Missing Layer FF marker");
            isValid = false;
        }
        
        // Layer IDチェック
        int layer_id = buffer_v[endMarkerPos + 5];
        if (!CheckLayerID(layer_id)) {
            isValid = false;
        }
    }
    
    // Cycle IDとTrigger IDが存在するかチェック
    if (buffer_v.size() >= 5) {
        // これらの値は、実際に2バイトから32ビットCycle IDと16ビットTrigger IDに
        // 変換されるので、ここでは簡易チェックのみ
        // cycleID = buffer_v[2]*0x10000 + buffer_v[3];
        // triggerID = buffer_v[4];
        
        // サイズチェックのみ行う
    } else {
        LogError("SPIROC bag too small to contain Cycle ID and Trigger ID");
        isValid = false;
    }
    
    if (!isValid) {
        totalErrors.spirocBagErrors++;
        errorsByEvent[currentEvent].spirocBagErrors++;
    }
    
    return isValid;
}

bool DataQualityChecker::CheckChipBuffer(const std::vector<int>& chip_v) {
    bool isValid = true;
    
    // チップバッファのサイズチェック
    if (!CheckChipBufferSize(chip_v)) {
        isValid = false;
    }
    
    // チップIDチェック
    int size = chip_v.size();
    if (size > 0) {
        int chip_id = chip_v[size-1];
        if (!CheckChipID(chip_id)) {
            isValid = false;
        }
    }
    
    // チャンネルデータのチェック
    int memo_no = (size - 1) / 73;
    
    for (int memo = 0; memo < memo_no; memo++) {
        int offset = memo * 73;
        
        for (int ch = 0; ch < 36; ch++) {
            // TDCデータチェック
            int tdc = chip_v[offset + ch] & 0x0FFF;
            int hit = (chip_v[offset + ch] & 0x1000) >> 12;
            
            // ADCデータチェック
            int adc = chip_v[offset + ch + 36] & 0x0FFF;
            int gain = (chip_v[offset + ch + 36] & 0x2000) >> 13;
            
            // 値の範囲チェック
            if (tdc < 0 || tdc > 4095 || adc < 0 || adc > 4095) {
                std::ostringstream os;
                os << "Invalid channel data: memo=" << memo << ", ch=" << ch 
                   << ", tdc=" << tdc << ", adc=" << adc;
                LogError(os.str());
                totalErrors.channelDataErrors++;
                errorsByEvent[currentEvent].channelDataErrors++;
                isValid = false;
            }
        }
        
        // BCIDチェック
        int bcid = chip_v[offset + 72];
        if (bcid < 0) {
            std::ostringstream os;
            os << "Invalid BCID: " << bcid;
            LogError(os.str());
            isValid = false;
        }
    }
    
    if (!isValid) {
        totalErrors.chipBufferErrors++;
        errorsByEvent[currentEvent].chipBufferErrors++;
    }
    
    return isValid;
}

void DataQualityChecker::AnalyzeFile(const std::string& filename) {
    std::ifstream f_in(filename.c_str(), std::ios::binary);
    if (!f_in.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    // バッファサイズを制限（例: 10MB）
    const size_t BUFFER_SIZE = 10 * 1024 * 1024;
    std::vector<unsigned char> sliding_buffer(BUFFER_SIZE);
    
    // バッファリングされたイベント処理用
    std::vector<int> event_buffer;
    int prev_triggerID = -1;
    
    // スライディングウィンドウの状態追跡
    size_t bytes_read = 0;
    size_t window_start = 0;
    bool in_event = false;
    bool end_of_file = false;
    
    // ファイルをストリーミング処理
    while (!end_of_file) {
        // スライディングウィンドウをシフト
        if (window_start > 0) {
            // 残りのデータを先頭に移動
            size_t remaining = bytes_read - window_start;
            if (remaining > 0) {
                memmove(&sliding_buffer[0], &sliding_buffer[window_start], remaining);
            }
            bytes_read = remaining;
            window_start = 0;
        }
        
        // バッファを補充
        if (bytes_read < BUFFER_SIZE) {
            f_in.read((char*)(&sliding_buffer[bytes_read]), BUFFER_SIZE - bytes_read);
            size_t new_bytes = f_in.gcount();
            bytes_read += new_bytes;
            
            if (new_bytes == 0) {
                end_of_file = true;
                // 処理中のイベントがあれば完了させる
                if (in_event) {
                    LogError("Incomplete event at end of file");
                    
                    // 最後のイベントを処理
                    currentEvent++;
                    totalEvents++;
                    
                    CheckEventBag(event_buffer);
                    in_event = false;
                }
                break;
            }
        }
        
        // イベントマーカーをスキャン
        for (size_t i = window_start; i <= bytes_read - 4; i++) {
            // イベント開始マーカーを検出

            if (!in_event && 
                sliding_buffer[i] == 0xfb && sliding_buffer[i+1] == 0xee && 
                sliding_buffer[i+2] == 0xfb && sliding_buffer[i+3] == 0xee) {
                
                // 新しいイベントの開始
                if (!event_buffer.empty()) {
                    // 前のイベントを完了
                    CheckEventBag(event_buffer);
                }
                
                event_buffer.clear();
                for (size_t j = i; j < i + 4; j++) {
                    event_buffer.push_back(sliding_buffer[j]);
                }
                in_event = true;
                currentEvent++;
                totalEvents++;
                
                prev_triggerID = -1;
                window_start = i + 4;
                break;
            }    
            if (in_event) {
                // イベント中の場合、イベントデータを収集
                event_buffer.push_back(sliding_buffer[i]);
                // イベント終了マーカーを検出
                if (sliding_buffer[i] == 0xfe && sliding_buffer[i+1] == 0xdd && 
                    sliding_buffer[i+2] == 0xfe && sliding_buffer[i+3] == 0xdd) {
                    
                    // 終了マーカーを含める
                    for (int j = 1; j < 4; j++) {
                        event_buffer.push_back(sliding_buffer[i+j]);
                    }
                    
                    // イベントを処理
                    CheckEventBag(event_buffer);
                    
                    // SPIROCバッグ処理
                    ProcessSPIROCBags(event_buffer, prev_triggerID);
                    event_buffer.clear();
                    in_event = false;
                    window_start = i + 4;
                    break;
                    
                } else if (i > window_start && sliding_buffer[i] == 0xfb && sliding_buffer[i+1] == 0xee && 
                           sliding_buffer[i+2] == 0xfb && sliding_buffer[i+3] == 0xee) {
                    // 新しいイベント開始マーカーが見つかった場合
                    LogError("Nested event start marker found");
                    event_buffer.erase(event_buffer.end()-1, event_buffer.end());
                    // 途中までのデータでイベントを処理
                    CheckEventBag(event_buffer);
                    // SPIROCバッグ処理
                    ProcessSPIROCBags(event_buffer, prev_triggerID);
                    event_buffer.clear();
                    in_event = false;
                    window_start = i;
                    break;
                    
                }else if (i == bytes_read - 4) {
                    // バッファの終わりまでエンドマーカーが見つからなかった場合
                    // イベントは続いている
                    for (size_t j = window_start; j < bytes_read; j++) {
                        event_buffer.push_back(sliding_buffer[j]);
                    }
                    window_start = bytes_read;
                    break;
                    
                } else if (i - window_start >= BUFFER_SIZE - 8) {
                    // イベントが異常に大きい場合（バッファサイズを超える）
                    LogError("Event too large, may be corrupt");
                    
                    // 途中までのデータでイベントを処理
                    for (size_t j = window_start; j < i; j++) {
                        event_buffer.push_back(sliding_buffer[j]);
                    }
                    
                    CheckEventBag(event_buffer);
                    event_buffer.clear();
                    
                    in_event = false;
                    window_start = i;
                    break;
                }
            }
        }
        
        // ウィンドウ終端の処理
        if (window_start == bytes_read) {
            // バッファを完全に消費した
            window_start = 0;
            bytes_read = 0;
        }
    }
    
    f_in.close();
}

void DataQualityChecker::ProcessSPIROCBags(const std::vector<int>& event_buffer, int& prev_triggerID) {
    // SPIROCバッグの処理をイベントバッファから抽出
    size_t pos = 4; // イベントヘッダーをスキップ
    size_t event_end = event_buffer.size() - 4; // イベントエンドマーカーをスキップ
    
    while (pos < event_end) {
        // SPIROCバッグのStartマーカーを探す
        bool found_spiroc_start = false;
        size_t spiroc_start = 0;
        
        for (size_t j = pos; j <= event_end - 4; ++j) {
            if (event_buffer[j] == 0xfa && event_buffer[j+1] == 0x5a && 
                event_buffer[j+2] == 0xfa && event_buffer[j+3] == 0x5a) {
                found_spiroc_start = true;
                spiroc_start = j;
                break;
            }
        }
        
        if (!found_spiroc_start) break;
        
        // SPIROCバッグのEndマーカーを探す
        bool found_spiroc_end = false;
        size_t spiroc_end = 0;
        
        for (size_t j = spiroc_start + 4; j <= event_end - 4; ++j) {
            if (event_buffer[j] == 0xfe && event_buffer[j+1] == 0xee && 
                event_buffer[j+2] == 0xfe && event_buffer[j+3] == 0xee) {
                found_spiroc_end = true;
                spiroc_end = j + 3;
                break;
            }
        }
        
        if (!found_spiroc_end) {
            LogError("SPIROC bag end marker not found");
            break;
        }
        
        // SPIROCバッグを抽出
        std::vector<int> spiroc_buffer;
        for (size_t j = spiroc_start; j <= spiroc_end; ++j) {
            spiroc_buffer.push_back(event_buffer[j]);
        }
        
        // SPIROCバッグの品質チェック
        CheckSPIROCBag(spiroc_buffer);
        
        // Layer FF markerとLayer IDをチェック
        if (spiroc_end + 2 <= event_end) {
            if (event_buffer[spiroc_end + 1] != 0xff) {
                LogError("Missing Layer FF marker after SPIROC bag");
            }
            
            int layer_id = event_buffer[spiroc_end + 2];
            CheckLayerID(layer_id);
        }
        
        // Trigger IDを抽出して一貫性をチェック
        if (spiroc_buffer.size() >= 5) {
            int triggerID = spiroc_buffer[4];
            CheckTriggerIDConsistency(triggerID, prev_triggerID);
            prev_triggerID = triggerID;
        }
        
        // 次のSPIROCバッグ位置へ
        pos = spiroc_end;
    }
}
void DataQualityChecker::PrintSummary() {
    std::cout << "Total Events Processed: " << totalEvents << std::endl;
    std::cout << "Total Errors:" << std::endl;
    std::cout << "  Event Bag Errors: " << totalErrors.eventBagErrors << std::endl;
    std::cout << "  SPIROC Bag Errors: " << totalErrors.spirocBagErrors << std::endl;
    std::cout << "  Chip Buffer Errors: " << totalErrors.chipBufferErrors << std::endl;
    std::cout << "  Layer ID Errors: " << totalErrors.layerIDErrors << std::endl;
    std::cout << "  Chip ID Errors: " << totalErrors.chipIDErrors << std::endl;
    std::cout << "  Marker Errors: " << totalErrors.markerErrors << std::endl;
    std::cout << "  Size Errors: " << totalErrors.sizeErrors << std::endl;
    std::cout << "  Trigger ID Errors: " << totalErrors.triggerIDErrors << std::endl;
    std::cout << "  Cycle ID Errors: " << totalErrors.cycleIDErrors << std::endl;
    std::cout << "  Channel Data Errors: " << totalErrors.channelDataErrors << std::endl;
}
void DataQualityChecker::WriteReport(const std::string& filename) {
    std::ofstream report_file(filename);
    if (!report_file.is_open()) {
        std::cerr << "Error: Cannot open report file " << filename << std::endl;
        return;
    }
    
    report_file << "Data Quality Check Report\n";
    report_file << "=========================\n";
    report_file << "Total Events Processed: " << totalEvents << "\n\n";
    
    report_file << "Total Errors:\n";
    report_file << "  Event Bag Errors: " << totalErrors.eventBagErrors << "\n";
    report_file << "  SPIROC Bag Errors: " << totalErrors.spirocBagErrors << "\n";
    report_file << "  Chip Buffer Errors: " << totalErrors.chipBufferErrors << "\n";
    report_file << "  Layer ID Errors: " << totalErrors.layerIDErrors << "\n";
    report_file << "  Chip ID Errors: " << totalErrors.chipIDErrors << "\n";
    report_file << "  Marker Errors: " << totalErrors.markerErrors << "\n";
    report_file << "  Size Errors: " << totalErrors.sizeErrors << "\n";
    report_file << "  Trigger ID Errors: " << totalErrors.triggerIDErrors << "\n";
    report_file << "  Cycle ID Errors: " << totalErrors.cycleIDErrors << "\n";
    report_file << "  Channel Data Errors: " << totalErrors.channelDataErrors << "\n\n";
    
    if (!errorLog.empty()) {
        report_file << "Detailed Error Log:\n";
        for (const auto& error : errorLog) {
            report_file << error << "\n";
        }
    } else {
        report_file << "No errors found.\n";
    }
    
    report_file.close();
}