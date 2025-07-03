#ifndef DATAQUALITYCHECKER_H
#define DATAQUALITYCHECKER_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <map>

class DataQualityChecker {
public:
    DataQualityChecker();
    ~DataQualityChecker();
    
    // イベントデータのチェック
    bool CheckEventBag(const std::vector<int>& buffer_v);
    bool CheckSPIROCBag(const std::vector<int>& buffer_v);
    bool CheckChipBuffer(const std::vector<int>& chip_v);
    
    // 全データファイルの解析
    void AnalyzeFile(const std::string& filename);
    
    // 結果の出力
    void PrintSummary();
    void WriteReport(const std::string& filename);

private:
    // エラーカウンター
    struct ErrorCounts {
        int eventBagErrors;      // イベントバッグのエラー
        int spirocBagErrors;     // SPIROCバッグのエラー
        int chipBufferErrors;    // チップバッファのエラー
        int layerIDErrors;       // レイヤーIDのエラー
        int chipIDErrors;        // チップIDのエラー
        int markerErrors;        // マーカーのエラー
        int sizeErrors;          // サイズのエラー
        int triggerIDErrors;     // トリガーIDの不一致
        int cycleIDErrors;       // サイクルIDのエラー
        int channelDataErrors;   // チャンネルデータのエラー
        
        ErrorCounts() : eventBagErrors(0), spirocBagErrors(0), chipBufferErrors(0),
                        layerIDErrors(0), chipIDErrors(0), markerErrors(0),
                        sizeErrors(0), triggerIDErrors(0), cycleIDErrors(0),
                        channelDataErrors(0) {}
    };
    
    ErrorCounts totalErrors;
    std::map<int, ErrorCounts> errorsByEvent; // イベント番号ごとのエラー
    
    int totalEvents;
    int currentEvent;
    
    // 詳細なエラーログ
    std::vector<std::string> errorLog;
    
    // 内部チェック関数
    bool CheckEventMarkers(const std::vector<int>& buffer_v);
    bool CheckSPIROCMarkers(const std::vector<int>& buffer_v);
    bool CheckLayerID(int layer_id);
    bool CheckChipID(int chip_id);
    bool CheckSPIROCSize(const std::vector<int>& buffer_v);
    bool CheckChipBufferSize(const std::vector<int>& chip_v);
    bool CheckTriggerIDConsistency(int triggerID, int prev_triggerID);
    void ProcessSPIROCBags(const std::vector<int>& event_buffer, int& prev_triggerID);
    void LogError(const std::string& message);
};

#endif // DATAQUALITYCHECKER_H