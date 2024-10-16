idf_component_register(SRCS "main.c" "src/tasks/wifi_task.c" "src/tasks/HAR_task.c" "src/tasks/predictor_task.cc" "src/model/model.c" 
                            "src/tasks/csi_task.c" "src/tasks/processor_task.c" "common/custom_queue.c" "src/filters/filters.c"
                    INCLUDE_DIRS ".")