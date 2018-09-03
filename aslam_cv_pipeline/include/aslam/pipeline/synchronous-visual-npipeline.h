#ifndef SYN_VISUAL_NPIPELINE_H_
#define SYN_VISUAL_NPIPELINE_H_

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <opencv2/core/core.hpp>

#include <aslam/cameras/ncamera.h>
#include <aslam/common/macros.h>
#include <aslam/common/thread-pool.h>
#include <aslam/frames/visual-frame.h>
#include <aslam/frames/visual-nframe.h>
#include <aslam/pipeline/visual-pipeline.h>

namespace aslam {

/// \class VisualNPipeline
/// \brief An interface for pipelines that turn images into VisualNFrames
///
/// This is the abstract interface for visual pipelines that turn raw images
/// into VisualNFrame data. The underlying pipeline may include undistortion
/// or rectification, image contrast enhancement, feature detection and
/// descriptor computation, or other operations.
///
/// The class has two NCameras calibration structs that represent the
/// intrinsic and extrinsic calibration of the camera system.
/// The "input" calibration (getInputNCameras()) represents the calibration of
/// raw camera system, before any image processing, resizing, or undistortion
/// has taken place. The "output" calibration (getOutputNCameras())
/// represents the calibration parameters of the images and keypoints that get
/// set in the VisualNFrames struct. These are the camera parameters after
/// image processing, resizing, undistortion, etc.
///
/// The class should synchronize images with nearby timestamps and handle
/// out-of-order images. When all frames of a VisualNFrame are complete,
/// they are added to a list of output frames in the order that they are
/// completed. This list should be sorted by time (oldest first) and the number
/// of elements can be queried by numVisualNFramesComplete(). The getNext()
/// function retrieves the oldest complete VisualNFrames and leaves the remaining.
/// The getLatestAndClear() function gets the newest VisualNFrames and discards
/// anything older.
class SynchronousVisualNPipeline final {
 public:
  ASLAM_POINTER_TYPEDEFS(SynchronousVisualNPipeline);
  ASLAM_DISALLOW_EVIL_CONSTRUCTORS(SynchronousVisualNPipeline);

  /// \brief Initialize a working pipeline.
  ///
  /// \param[in] num_threads            The number of processing threads.
  /// \param[in] pipelines              The ordered image pipelines, one pipeline
  ///                                   per camera in the same order as they are
  ///                                   indexed in the camera system.
  /// \param[in] input_camera_system    The camera system of the raw images.
  /// \param[in] output_camera_system   The camera system of the processed images.
  /// \param[in] timestamp_tolerance_ns How close should two image timestamps be
  ///                                   for us to consider them part of the same
  ///                                   synchronized frame?
  SynchronousVisualNPipeline(
      const std::vector<VisualPipeline::Ptr>& pipelines,
      const NCamera::Ptr& input_camera_system,
      const NCamera::Ptr& output_camera_system,
      const int64_t timestamp_tolerance_ns);

  ~SynchronousVisualNPipeline() = default;

  /// \brief Add an image to the visual pipeline
  ///
  /// This function is called by a user when an image is received.
  /// The pipeline then processes the images and constructs VisualNFrames
  ///
  /// \param[in] camera_index The index of the camera that this image corresponds to
  /// \param[in] image the image data
  /// \param[in] timestamp the time in integer nanoseconds.
  void processImage(
      const size_t camera_index, const cv::Mat& image,
      const int64_t timestamp_nanoseconds, VisualNFrame::Ptr* complete_nframe);

  /// Get the number of frames being processed.
  size_t getNumFramesProcessing() const;

  /// Get the input camera system that corresponds to the images
  /// passed in to processImage().
  /// Because this pipeline may do things like image undistortion or
  /// rectification, the input and output camera systems may not be the same.
  std::shared_ptr<const NCamera> getInputNCameras() const;

  /// Get the output camera system that corresponds to the VisualNFrame
  /// data that comes out.
  /// Because this pipeline may do things like image undistortion or
  /// rectification, the input and output camera systems may not be the same.
  std::shared_ptr<const NCamera> getOutputNCameras() const;


  /// \brief  Create a test visual npipeline.
  ///
  /// @param[in]  num_cameras   The number of cameras in the pipeline (determines the number of
  ///                           frames).
  /// @param[in]  num_threads   The number of threads used in the pipeline for processing the data.
  /// @param[in]  timestamp_tolerance_ns  Timestamp tolerance for frames to be considered
  ///                                     belonging together. [ns]
  /// @return  Pointer to the visual npipeline.
  static SynchronousVisualNPipeline::Ptr createTestSynchronousVisualNPipeline(
      const size_t num_cameras, const int64_t timestamp_tolerance_ns);

 private:

  /// One visual pipeline for each camera.
  std::vector<std::shared_ptr<VisualPipeline>> pipelines_;

  typedef std::map<int64_t, std::shared_ptr<VisualNFrame>> TimestampVisualNFrameMap;
  /// The frames that are in progress.
  TimestampVisualNFrameMap processing_;


  /// The camera system of the raw images.
  std::shared_ptr<NCamera> input_camera_system_;
  /// The camera system of the processed images.
  std::shared_ptr<NCamera> output_camera_system_;

  /// The tolerance for associating host timestamps as being captured at the same time
  int64_t timestamp_tolerance_ns_;
};
}  // namespace aslam
#endif // VISUAL_NPIPELINE_H_