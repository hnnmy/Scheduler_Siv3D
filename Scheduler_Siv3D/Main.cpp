#include <Siv3D.hpp>
#include "SyllabusFetcher.hpp"

void Main()
{
	// 0. Load Config
	AppConfig appConfig;
	const INI ini{ U"config.ini" };

	if (ini)
	{
		appConfig.vpsUrl = ini[U"Server.Url"];
		appConfig.apiKey = ini[U"Server.ApiKey"];
		if (appConfig.vpsUrl.isEmpty() || appConfig.apiKey.isEmpty())
		{
			// Fallback warning if keys are missing
			Print << U"[Warning] API Key or URL missing in config.ini";
		}
		else
		{
			appConfig.valid = true;
		}
	}
	else
	{
		Print << U"[Error] Could not load config.ini";
		Print << U"Make sure config.ini exists in the App folder.";
	}

	// 1. Setup Window
	Window::Resize(900, 600);
	Window::SetTitle(U"Sophia Scheduler - Cloud Edition");
	Scene::SetBackground(Palette::White);

	// 2. Register Fonts
	FontAsset::Register(U"Medium", 30);
	FontAsset::Register(U"Small", 16);
	FontAsset::Register(U"Detail", 14);

	// 3. Application State
	SyllabusFetcher fetcher;
	TextEditState codeInput;
	codeInput.text = U"EMG54700";

	bool isFetching = false;
	String statusMessage = U"Enter a course code and click Fetch.";
	Array<SyllabusData> myCourses;

	while (System::Update())
	{
		// --- INPUT UI ---
		SimpleGUI::TextBox(codeInput, Vec2{ 20, 20 }, 150);

		if (SimpleGUI::Button(U"Fetch", Vec2{ 180, 20 }) && !isFetching)
		{
			if (!appConfig.valid)
			{
				statusMessage = U"Error: Invalid Configuration (Check config.ini)";
			}
			else if (codeInput.text.isEmpty())
			{
				statusMessage = U"Please enter a valid code.";
			}
			else
			{
				fetcher.startFetch(codeInput.text, U"2025", appConfig);
				isFetching = true;
				statusMessage = U"Querying Cloud Server...";
			}
		}

		// --- ASYNC HANDLING ---
		if (isFetching)
		{
			// Spinner
			Circle{ 400, 300, 30 }.drawArc(Scene::Time() * 180_deg, 270_deg, 4, 4, Palette::Skyblue);

			if (fetcher.hasResult())
			{
				SyllabusData data = fetcher.getResult();
				isFetching = false;

				if (data.isValid)
				{
					myCourses << data;
					statusMessage = U"Success! Added " + data.title;
				}
				else
				{
					statusMessage = U"Error: Course not found or API error.";
				}
			}
		}

		// --- RENDER LIST ---
		FontAsset(U"Medium")(statusMessage).draw(20, 60, Palette::Black);

		int y = 100;
		for (const auto& course : myCourses)
		{
			Rect cardRect{ 20, y, 860, 80 };
			bool isHovered = cardRect.mouseOver();

			// Draw Card
			cardRect.draw(isHovered ? Palette::Azure : Palette::White).drawFrame(1, Palette::Lightgray);

			// Title
			FontAsset(U"Medium")(course.title).draw(30, y + 5, Palette::Black);

			// Sub Info (Using correct member names)
			String subInfo = U"{} | {} | {}"_fmt(course.instructor, course.schedule, course.classroom);
			FontAsset(U"Small")(subInfo).draw(30, y + 45, Palette::Gray);

			// Tooltip on Hover (Show count of extra details)
			if (isHovered)
			{
				SimpleGUI::Headline(U"Extra Details: {}"_fmt(course.details.size()), Vec2{ 750, (double)y + 25 });
			}

			y += 90;
		}
	}
}
