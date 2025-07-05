

//std::ofstream file;
#include <Pulse.hpp>
#include <limits>


#include "patest.hpp"
#include "ChunkedArray.hpp"

// Pulse audio testing

TestApp::TestApp()
: Gio::Application("de.pfeifer_syscon.pulseTest")
{
}

void
TestApp::start(GMainContext* ctx)
{
    m_pulse = std::make_shared<PulseIn>(ctx);
}

void
TestApp::on_activate()
{
    Gio::Application::on_activate();
    // create main loop so the dispatch works
    auto main = Glib::MainLoop::create(false);
    auto ctx = main->get_context();
    GMainContext* c_ctx = ctx->gobj();
    ctx->signal_idle().connect_once(
            sigc::bind(
              sigc::mem_fun(*this, &TestApp::start)
            , c_ctx));
    ctx->signal_timeout().connect_seconds_once([&] {
            main->quit();
    }, 12);
    main->run();
}



int
TestApp::getResult()
{
    auto data = m_pulse->read();
    std::cout << "Got " << data.size() << " samples" << std::endl;
    int16_t min{std::numeric_limits<int16_t>::max()},max{std::numeric_limits<int16_t>::min()};
    int64_t avg{},cnt{};
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < data.size(); ++i) {
        auto v = data.operator [](i);
        min = std::min(min, v);
        max = std::max(max, v);
        avg += v;
        ++cnt;
    }
    auto finish = std::chrono::steady_clock::now();
    double elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
    std::cout << "duration " << elapsed_seconds << std::endl;
    std::cout << "min " << min
              << " max " << max
              << " avg " << static_cast<float>(avg)/static_cast<float>(cnt) << std::endl;

    return 0;
}


int main(int argc, char **argv)
{
    std::setlocale(LC_ALL, "");      // make locale dependent, and make glib accept u8 const !!!
    Glib::init();
    Gio::init();

    auto app = TestApp();
    app.run(argc, argv);
    return app.getResult();

//    file.open("out.pcm", std::ios::binary | std::ios::trunc);
//
//    pa_mainloop *loop = pa_mainloop_new();
//    pa_mainloop_api *api = pa_mainloop_get_api(loop);
//    pa_context *ctx = pa_context_new(api, "padump");
//    pa_context_set_state_callback(ctx, &pa_context_notify_cb, nullptr /*userdata*/);
//    if (pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
//	std::cerr << "PA connection failed.\n";
//	return -1;
//    }
//
//
//    pa_mainloop_run(loop, nullptr);
//
//    // pa_stream_disconnect(..)
//
//    pa_context_disconnect(ctx);
//    pa_mainloop_free(loop);
//    file.close();
//    return 0;
}