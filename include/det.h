#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <vector>

#include <cstring>
#include <fstream>
#include <numeric>

#include <include/utility.h>
#include <include/preprocess_op.h>
#include <include/postprocess_op.h>
#include <openvino/openvino.hpp>
#include <openvino/core/preprocess/pre_post_process.hpp>

namespace PaddleOCR {

class Det
{
public:
    Det();
    ~Det();

    bool init(cv::Mat &src_img, std::string model_path);
    bool run(std::vector<OCRPredictResult> &ocr_results);

private:

    ov::InferRequest infer_request;
    std::string model_path;
    cv::Mat src_img;
    shared_ptr<ov::Model> model;
    ov::CompiledModel det_model;
    float ratio_h{};
    float ratio_w{};
    cv::Mat resize_img;
    string limit_type_ = "max";
    int limit_side_len_ = 960;
    double e = 1.0 / 255.0;
    
    std::vector<float> mean_ = {0.485f, 0.456f, 0.406f};
    std::vector<float> scale_ = {0.229f, 0.224f, 0.225f};

    double det_db_thresh_ = 0.3;
    double det_db_box_thresh_ = 0.5;
    double det_db_unclip_ratio_ = 2.0;
    std::string det_db_score_mode_ = "slow";
    bool use_dilation_ = false;

    // resize
    ResizeImgType0 resize_op_;
    // post-process
    DBPostProcessor post_processor_;
};
}