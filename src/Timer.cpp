#include "chrono"

class Timer{
public:
      void start(){
        stopped = false;
        startTime = std::chrono::high_resolution_clock::now();;
        lastTime = startTime;
        delta = 0.0f;
        elapsed = 0.0f;
      }

      void update(){
        if (!stopped) {
          auto now = std::chrono::high_resolution_clock::now();
          delta = std::chrono::duration<float>(now - lastTime).count();
          elapsed = std::chrono::duration<float>(now - startTime).count();
          lastTime = now;
        }
      }

      float getElapsedTime(){
        return elapsed;
      }

      float getDelta() {
        return delta;
      }
      void stop() {
        stopped = true;
      }

      void resume() {
        if (stopped) {
          stopped = false;
          // Update lastTime to current time to avoid large delta when resuming
          lastTime = std::chrono::high_resolution_clock::now();
          // Adjust startTime to maintain the same elapsed time
          auto pauseDuration = lastTime - startTime;
          startTime = lastTime - std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
            std::chrono::duration<float>(elapsed));
        }
      }

    private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point lastTime;
    float delta = 0.0f;
    float elapsed = 0.0f;
    bool stopped = false;
  };