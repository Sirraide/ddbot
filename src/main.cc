#include "token.h"

#include <clocale>
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>
#include <future>
#include <iostream>
#include <list>
#include <random>

using namespace std;
using namespace dpp;

#define repeat(N_) for (size_t iter__ = 0, until__ = N_; iter__ < until__; iter__++)

template <typename T>
struct triple {
	T a;
	T b;
	T c;

	constexpr triple() = default;
	constexpr triple(T a, T b, T c) : a(a), b(b), c(c) {}
	constexpr triple(unsigned val) : a(val >> 16 & 0xff), b(val >> 8 & 0xff), c(val & 0xff) {}

	[[nodiscard]] constexpr inline unsigned colour() const { return a << 16 | b << 8 | c; }
};

constexpr const triple<uint8_t> green{109, 58, 59};
constexpr const triple<uint8_t> red{0, 56, 54};

cluster		  bot(TOKEN);
random_device rd;

// clang-format off
template <typename T = int>
inline T tryval(const interaction_create_t& event, const string& name, T _default) noexcept {
	try { return get<T>(event.get_parameter(name)); } catch (const exception& ignored) { return _default; }
}

// clang-format on

template <typename T = unsigned>
inline bool trystrtol(T* i, const string& str) {
	try {
		*i = strtol(str.c_str(), nullptr, 10);
		return true;
	} catch (const std::exception& ignored) { return false; }
}

constexpr triple<uint8_t> hsl2rgb(triple<uint8_t> hsl) {
	double h = hsl.a, s = hsl.b, l = hsl.c;

	s /= 100.0;
	l /= 100.0;

	double c   = (1.0 - abs(2.0 * l - 1.0)) * s;
	double h60 = h / 60.0;
	double x   = c * (1.0 - abs(fmod(h60, 2.0) - 1.0));
	double m   = l - c / 2.0;

	triple<double> rgb{};

	switch (int(h60)) {
		case 0: rgb = {c, x, 0}; break;
		case 1: rgb = {x, c, 0}; break;
		case 2: rgb = {0, c, x}; break;
		case 3: rgb = {0, x, c}; break;
		case 4: rgb = {x, 0, c}; break;
		default: rgb = {c, 0, x};
	}

	triple<uint8_t> rgbret{};

	rgbret.a = round((rgb.a + m) * 255);
	rgbret.b = round((rgb.b + m) * 255);
	rgbret.c = round((rgb.c + m) * 255);

	return rgbret;
}

constexpr triple<uint8_t> lerp(triple<uint8_t> from, triple<uint8_t> to, double ratio) {
	double a = lerp(from.a, to.a, ratio);
	double b = lerp(from.b, to.b, ratio);
	double c = lerp(from.c, to.c, ratio);
	return {uint8_t(a), uint8_t(b), uint8_t(c)};
}

constexpr unsigned CalculateColour(unsigned die, unsigned total, unsigned num) {
	return hsl2rgb(lerp(red, green, double(total - num) / double(die * num - num))).colour();
}

message roll(int die, int num, const string& name, const string& url, snowflake channel_id) {
	default_random_engine			   rand{rd()};
	uniform_int_distribution<unsigned> distribution(1, die);

	string	 result;
	unsigned total = 0;

	bool first = true;
	repeat (num) {
		if (!first) result += ", ";
		auto val   = distribution(rand);
		bool natural = val == die;
		result += fmt::format("{}{}{}", natural ? "**" : "", val, natural ? "**" : "");
		total += val;
		first = false;
	}

	embed e;
	e.set_author(fmt::format("{} rolled {}d{}", name, num, die), "", url);
	e.set_description(result);
	e.set_color(CalculateColour(die, total, num));
	if (num > 1) e.add_field("Total", to_string(total));

	return {channel_id, e};
}

vector<string> split(const string& str) {
	string		   buf;
	vector<string> ret;

	for (auto& c : str) {
		if (isspace(c)) {
			if (!buf.empty()) ret.push_back(buf);
			buf.clear();
			continue;
		}
		buf += c;
	}
	return ret;
}

#define RegisterCommand(command)                                                             \
	bot.guild_command_create(command, server_id, [&](const confirmation_callback_t& event) { \
		if (event.type == "slashcommand") {                                                  \
			const auto& comm = get<slashcommand>(event.value);                               \
			cerr << "registered command '" << #command << "' with id " << comm.id << "\n";   \
		}                                                                                    \
	})

void RegisterCommands() {
	slashcommand roll;

	roll.set_name("roll")
		.set_description("Roll one or multiple dice")
		.set_application_id(bot.me.id)
		.add_option(command_option{co_integer, "die", "The type of die to roll (default: d20)"}
						.add_choice({"d4", 4})
						.add_choice({"d6", 6})
						.add_choice({"d8", 8})
						.add_choice({"d10", 10})
						.add_choice({"d12", 12})
						.add_choice({"d20", 20})
						.add_choice({"d100", 100}))
		.add_option(command_option{co_integer, "amount", "How many dice to roll (default: 1)"});
	RegisterCommand(roll);
}

void reply(message* msg, message answer) {
	answer.set_reference(msg->id);
	bot.message_create(answer);
}

void InitialiseCommandHandler() {
	bot.on_interaction_create([](const interaction_create_t& event) {
		if (event.command.type != it_application_command) return;
		auto data = get<command_interaction>(event.command.data);
		if (data.name == "roll") {
			event.reply(ir_acknowledge, "Rolling...");

			int		  die		 = tryval(event, "die", 20);
			int		  num		 = clamp(tryval(event, "amount", 1), 1, 1000);
			string	  name		 = event.command.member.nickname;
			string	  url		 = event.command.usr.get_avatar_url();
			snowflake channel_id = event.command.channel_id;

			if (name.empty()) name = event.command.usr.username;

			event.reply(ir_channel_message_with_source, roll(die, num, name, url, channel_id));
		}

		event.reply(ir_channel_message_with_source, fmt::format("Error: unknown command '{}'", data.name));
	});

	bot.on_message_create([](const message_create_t& event) {
		if (event.msg->content.starts_with("!roll")) {
			auto	 argv = split(event.msg->content);
			auto	 argc = argv.size();
			int die = 20, num = 1;

			if (argc > 1) {
				if(!trystrtol(&die, argv[1])) {
					reply(event.msg, fmt::format("**Error:** '{}' is not a number", argv[1]));
					return;
				}
			}
			if(argc > 2) {
				if(!trystrtol(&num, argv[2])) {
					reply(event.msg, fmt::format("**Error:** '{}' is not a number", argv[2]));
					return;
				}
				clamp(num, 1, 1000);
			}
			string	  url;
			string	  name		 = event.msg->member.nickname;

			promise<void> prom;
			bot.user_get(event.msg->member.user_id, [&](const confirmation_callback_t & ev) {
				url = get<user>(ev.value).get_avatar_url();
				prom.set_value();
			});
			auto f = prom.get_future();
			f.get();

			reply(event.msg, roll(die, num, name, url, event.msg->channel_id));

		}
	});
}

int main() {
	setlocale(LC_ALL, "");

	InitialiseCommandHandler();

	bot.on_ready([](const ready_t& event) {
		(void) event;
		cerr << "d&dbot is online\n";
		RegisterCommands();
	});

	bot.start(false);
}
