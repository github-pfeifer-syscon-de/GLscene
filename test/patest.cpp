

//std::ofstream file;
#include <Pulse.hpp>
#include <limits>


#include "patest.hpp"
#include "ChunkedArray.hpp"

// Pulse audio testing
//   beware this uses real in-/output
//   for that reason it is not called by default

TestApp::TestApp()
: Gio::Application("de.pfeifer_syscon.pulseTest")
{
}

void
TestApp::start(const Glib::RefPtr<Glib::MainContext>& ctx)
{
    auto pulseCtx = std::make_shared<psc::snd::PulseCtx>(ctx);
    psc::snd::PulseFormat fmt;
    m_pulseIn = std::make_shared<psc::snd::PulseIn>(pulseCtx, fmt);
    auto sineSource = std::make_shared<psc::snd::AudioGenerator>();
    sineSource->setFrequency(441.0f);
    m_pulseOut = std::make_shared<psc::snd::PulseOut>(pulseCtx, fmt, sineSource);
}

void
TestApp::on_activate()
{
    Gio::Application::on_activate();
    // create main loop so the dispatch works
    auto main = Glib::MainLoop::create(false);
    auto ctx = main->get_context();
    ctx->signal_idle().connect_once(
            sigc::bind(
              sigc::mem_fun(*this, &TestApp::start)
            , ctx));
    ctx->signal_timeout().connect_seconds_once([&] {
            main->quit();
    }, 6);
    main->run();
}



int
TestApp::getResult()
{
    m_pulseOut->drain();
    m_pulseIn->disconnect();
    auto data = m_pulseIn->read();
    printf("TestApp::getResult got %ld samples\n", data.size());
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


}