#ifndef ASLAM_TRACKING_HELPERS_H_
#define ASLAM_TRACKING_HELPERS_H_

#include <vector>

#include <aslam/common/memory.h>
#include <aslam/common/pose-types.h>
#include <glog/logging.h>
#include <Eigen/Core>
#include <opencv2/core/core.hpp>

namespace aslam {
class VisualFrame;
typedef aslam::Aligned<std::vector, Eigen::Vector2d>::type Verctor2dList;

void convertKeypointVectorToCvPointList(const Eigen::Matrix2Xd& keypoints,
                                        std::vector<cv::Point2f>* keypoints_cv);

void convertCvPointListToKeypointVector(const std::vector<cv::Point2f>& keypoints,
                                        Eigen::Matrix2Xd* keypoints_eigen);

/// Rotate keypoints from a VisualFrame using a specified rotation. Note that if the back-,
/// projection fails or the keypoint leaves the image region, the predicted keypoint will be left
/// unchanged and the prediction_success will be set to false.
void predictKeypointsByRotation(const VisualFrame& frame_k,
                                const aslam::Quaternion& q_Ckp1_Ck,
                                Eigen::Matrix2Xd* predicted_keypoints_kp1,
                                std::vector<unsigned char>* prediction_success);

/// Insert a list of keypoints into a VisualFrame.
void insertKeypointsIntoVisualFrame(const Eigen::Matrix2Xd& new_keypoints,
                                    const Eigen::VectorXd& new_keypoint_scores,
                                    const Eigen::VectorXd& new_keypoint_scales,
                                    const double& fixed_keypoint_uncertainty_px,
                                    aslam::VisualFrame* frame);

/// Append a list of kepoints to a VisualFrame.
void insertAdditionalKeypointsToVisualFrame(const Eigen::Matrix2Xd& new_keypoints,
                                            const double& fixed_keypoint_uncertainty_px,
                                            aslam::VisualFrame* frame);
void insertAdditionalKeypointsToVisualFrame(const Verctor2dList& keypoints,
                                            const double& fixed_keypoint_uncertainty_px,
                                            aslam::VisualFrame* frame);

}  // namespace aslam

#endif  // ASLAM_TRACKING_HELPERS_H_
