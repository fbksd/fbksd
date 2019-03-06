
# OpenCV
find_package(OpenCV REQUIRED)

add_executable(fbksd-mse mse.cpp)
target_link_libraries(fbksd-mse PRIVATE ${OpenCV_LIBS} fbksd::iqa)
install(TARGETS fbksd-mse
    RUNTIME DESTINATION bin
)

add_executable(fbksd-rmse rmse.cpp)
target_link_libraries(fbksd-rmse PRIVATE ${OpenCV_LIBS} fbksd::iqa)
install(TARGETS fbksd-rmse
    RUNTIME DESTINATION bin
)

add_executable(fbksd-psnr psnr.cpp)
target_link_libraries(fbksd-psnr PRIVATE ${OpenCV_LIBS} fbksd::iqa)
install(TARGETS fbksd-psnr
    RUNTIME DESTINATION bin
)

add_executable(fbksd-ssim ssim.cpp)
target_link_libraries(fbksd-ssim PRIVATE ${OpenCV_LIBS} fbksd::iqa)
install(TARGETS fbksd-ssim
    RUNTIME DESTINATION bin
)
