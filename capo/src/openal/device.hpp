#pragma once
#include <detail/interfaces.hpp>
#include <detail/unique.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

namespace capo::openal {
struct Buffer {
	struct Deleter {
		void operator()(ALuint const id) const;
	};

	Unique<ALuint, Deleter> id{};

	static Buffer generate();

	void write(Clip clip);
};

struct Source {
	struct Deleter {
		void operator()(ALuint const id) const;
	};

	Unique<ALuint, Deleter> id{};

	static Source make();

	State state() const;
	void play();
	void stop();
	void pause();
	float gain() const;
	void set_gain(float value);
	float pitch() const;
	void set_pitch(float value);
	Vec3 position() const;
	void set_position(Vec3 const& value);
	Vec3 velocity() const;
	void set_velocity(Vec3 const& value);
	float max_distance() const;
	void set_max_distance(float radius);
};

struct Streamable {
	virtual bool update() = 0;
};

class StreamUpdater {
  public:
	static constexpr auto poll_rate_v{std::chrono::milliseconds{5}};

	StreamUpdater& operator=(StreamUpdater&&) = delete;

	StreamUpdater();
	~StreamUpdater();

	void track(Streamable* source);
	void untrack(Streamable* source);

  private:
	void poll();

	std::unordered_set<Streamable*> m_sources{};
	std::mutex m_mutex{};
	std::thread m_thread{};
	std::atomic<bool> m_stop{};
};

class SoundSource : public detail::SoundSource {
  public:
	State state() const final;
	void play() final;
	void stop() final;
	void pause() final;

	bool is_looping() const final;
	void set_looping(bool value) final;
	float gain() const final;
	void set_gain(float value) final;
	float pitch() const final;
	void set_pitch(float value) final;
	Vec3 position() const final;
	void set_position(Vec3 const& value) final;
	Vec3 velocity() const final;
	void set_velocity(Vec3 const& value) final;
	float max_distance() const final;
	void set_max_distance(float radius) final;
	Duration cursor() const final;
	void seek(Duration point) final;

	void set_clip(Clip clip) final;

  private:
	void bind();
	void unbind();

	Buffer m_buffer{Buffer::generate()};
	openal::Source m_source{openal::Source::make()};
};

class StreamSource : public detail::StreamSource, public Streamable {
  public:
	StreamSource(StreamUpdater* updater);
	~StreamSource() override;

	State state() const final;
	void play() final;
	void stop() final;
	void pause() final;

	bool is_looping() const final { return m_loop; }
	void set_looping(bool value) final { m_loop = value; }
	float gain() const final;
	void set_gain(float value) final;
	float pitch() const final;
	void set_pitch(float value) final;
	Vec3 position() const final;
	void set_position(Vec3 const& value) final;
	Vec3 velocity() const final;
	void set_velocity(Vec3 const& value) final;
	float max_distance() const final;
	void set_max_distance(float radius) final;
	Duration cursor() const final;
	void seek(Duration point) final;

	void set_stream(Stream stream) final;
	bool has_stream() const final;

	bool update() final;

  private:
	static constexpr std::size_t buffering_v{3u};
	static constexpr std::size_t frames_per_buffer_v{4096};

	void play(std::scoped_lock<std::mutex> const&);
	void stop(std::scoped_lock<std::mutex> const&);

	Stream m_stream{};
	std::array<Buffer, buffering_v> m_buffers{};
	std::vector<FrameIndex> m_queued{};
	std::vector<Sample> m_samples{};
	StreamUpdater* m_updater{};
	mutable std::mutex m_mutex{};
	std::atomic<bool> m_loop{};
	State m_state{State::eIdle};

	openal::Source m_source{openal::Source::make()};
};

class Device : public detail::Device {
  public:
	bool init() final;

	std::unique_ptr<detail::SoundSource> make_sound_source() const final { return std::make_unique<SoundSource>(); }
	std::unique_ptr<detail::StreamSource> make_stream_source() const final { return std::make_unique<StreamSource>(&m_updater); }

	float gain() const final;
	void set_gain(float value) final;
	Vec3 velocity() const final;
	void set_velocity(Vec3 const& value) final;
	Vec3 position() const final;
	void set_position(Vec3 const& value) final;
	Orientation orientation() const final;
	void set_orientation(Orientation const& value) final;

  private:
	struct Deleter {
		void operator()(ALCdevice* ptr) const;
		void operator()(ALCcontext* ptr) const;
	};

	std::unique_ptr<ALCdevice, Deleter> m_device{};
	std::unique_ptr<ALCcontext, Deleter> m_context{};
	mutable StreamUpdater m_updater{};
};
} // namespace capo::openal
