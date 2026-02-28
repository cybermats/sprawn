#include <sprawn/frontend/application.h>

int main(int argc, char** argv) {
    const char* filepath = argc >= 2 ? argv[1] : "";
    return sprawn::run_application(filepath) ? 0 : 1;
}
