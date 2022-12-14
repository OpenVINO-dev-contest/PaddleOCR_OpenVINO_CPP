#include "include/det.h"

namespace PaddleOCR {

Det::Det() {}

Det::~Det() {}

bool Det::init(cv::Mat &src_img, std::string model_path)
{   
    ov::Core core;
    this->model_path = model_path;
    this->model = core.read_model(this->model_path);
    this->src_img = src_img;
    this->resize_op_.Run(this->src_img, resize_img, this->limit_type_,
                        this->limit_side_len_, this->ratio_h, this->ratio_w);

    this->model->reshape({1, 3, resize_img.rows, resize_img.cols});

    // preprocessing API
    ov::preprocess::PrePostProcessor prep(this->model);
    // declare section of desired application's input format
    prep.input().tensor()
        .set_layout("NHWC")
        .set_color_format(ov::preprocess::ColorFormat::BGR);
    // specify actual model layout
    prep.input().model()
        .set_layout("NCHW");
    prep.input().preprocess()
        .mean(this->mean_)
        .scale(this->scale_);
    // dump preprocessor
    std::cout << "Preprocessor: " << prep << std::endl;
    this->model = prep.build();
    this->det_model = core.compile_model(this->model, "CPU");
    this->infer_request = this->det_model.create_infer_request();
    return true;
}

bool Det::run(std::vector<OCRPredictResult> &ocr_results)
{
    std::vector<std::vector<std::vector<int>>> boxes;
    auto input_port = this->det_model.input();

    // -------- set input --------
    double e = 1.0 / 255.0;
    resize_img.convertTo(resize_img, CV_32FC3, e);
    ov::Tensor input_tensor(input_port.get_element_type(), input_port.get_shape(), (float*)resize_img.data);
    this->infer_request.set_input_tensor(input_tensor);
    // -------- start inference --------
    this->infer_request.infer();

    auto output = this->infer_request.get_output_tensor(0);
    const float *out_data = output.data<const float>();

    ov::Shape output_shape = this->model->output().get_shape();
    const size_t n2 = output_shape[2];
    const size_t n3 = output_shape[3];
    const int n = n2 * n3;

    std::vector<float> pred(n, 0.0);
    std::vector<unsigned char> cbuf(n, ' ');

    for (int i = 0; i < n; i++) {
        pred[i] = float(out_data[i]);
        cbuf[i] = (unsigned char)((out_data[i]) * 255);
    }

    cv::Mat cbuf_map(n2, n3, CV_8UC1, (unsigned char *)cbuf.data());
    cv::Mat pred_map(n2, n3, CV_32F, (float *)pred.data());

    const double threshold = this->det_db_thresh_ * 255;
    const double maxvalue = 255;
    cv::Mat bit_map;
    cv::threshold(cbuf_map, bit_map, threshold, maxvalue, cv::THRESH_BINARY);
    if (this->use_dilation_) {
    cv::Mat dila_ele =
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::dilate(bit_map, bit_map, dila_ele);
    }

    boxes = post_processor_.BoxesFromBitmap(
        pred_map, bit_map, this->det_db_box_thresh_, this->det_db_unclip_ratio_,
        this->det_db_score_mode_);

    boxes = post_processor_.FilterTagDetRes(boxes, this->ratio_h, this->ratio_w, this->src_img);
    for (int i = 0; i < boxes.size(); i++) {
        OCRPredictResult res;
        res.box = boxes[i];
        ocr_results.push_back(res);
    }
    // sort boex from top to bottom, from left to right
    Utility::sorted_boxes(ocr_results);
    return true;
}
}