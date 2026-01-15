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
	Window::Resize(1280, 720);
	Window::SetTitle(U"Sophia Scheduler - Phase 2");
	Scene::SetBackground(Palette::White);

	// 2. Register Fonts
	FontAsset::Register(U"Header", 24, Typeface::Bold);
	FontAsset::Register(U"Title", 20, Typeface::Bold);
	FontAsset::Register(U"Normal", 16);
	FontAsset::Register(U"Small", 14);

	// 3. Application State
	SyllabusFetcher fetcher;
	TextEditState codeInput;
	codeInput.text = U"EMG54700";

	bool isFetching = false;
	String statusMessage = U"Ready to search.";

	// --- Data Storage ---
		// mySchedule: The list of courses currently registered and saved.
		// searchResult: The temporary course found by the search bar.
	Array<SyllabusData> mySchedule = SyllabusFetcher::LoadSchedule();
	Optional<SyllabusData> searchResult = none;

	while (System::Update())
	{
		// -------------------------------------------------------------
		// LEFT PANEL: My Schedule (Width: 800)
		// -------------------------------------------------------------
		Rect{ 0, 0, 800, 720 }.draw(Palette::White);
		Rect{ 0, 0, 800, 60 }.draw(ColorF{ 0.2, 0.4, 0.8 }); // Header
		FontAsset(U"Header")(U"My Registered Courses").draw(20, 15, Palette::White);

		// List Header
		FontAsset(U"Small")(U"{} courses registered"_fmt(mySchedule.size())).draw(650, 22, Palette::White);

		// Render List
		{
			int y = 80;
			for (auto& course : mySchedule)
			{
				// Card Background
				Rect card{ 20, y, 760, 90 };
				bool hover = card.mouseOver();

				// [FIXED] Changed Palette::Rdpo (Typo) to Palette::Red
				Color typeColor = (course.type == CourseType::Required) ? Palette::Red : Palette::Skyblue;

				card.draw(hover ? Palette::Azure : Palette::White).drawFrame(1, Palette::Lightgray);
				Rect{ 20, y, 10, 90 }.draw(typeColor); // Left Strip

				// Info
				FontAsset(U"Title")(course.title).draw(40, y + 10, Palette::Black);
				FontAsset(U"Normal")(U"{} | {}"_fmt(course.instructor, course.schedule)).draw(40, y + 40, Palette::Gray);
				FontAsset(U"Small")(course.code).draw(40, y + 65, Palette::Lightgray);

				// --- ACTIONS ---

				// Toggle Type (Req/Elec)
				if (SimpleGUI::Button(course.type == CourseType::Required ? U"Required" : U"Elective", Vec2{ 600, (double)y + 10 }, 100))
				{
					// Switch Type
					course.type = (course.type == CourseType::Required) ? CourseType::Elective : CourseType::Required;
					SyllabusFetcher::SaveSchedule(mySchedule); // Auto-save
				}

				// Delete Button
				if (SimpleGUI::Button(U"Remove", Vec2{ 600, (double)y + 50 }, 100))
				{
					// Remove by ID matching (safest way)
					mySchedule.remove_if([&](const SyllabusData& c) { return c.code == course.code; });
					SyllabusFetcher::SaveSchedule(mySchedule); // Auto-save
					break; // Stop loop to avoid iterator invalidation
				}

				y += 100;
			}
		}


		// -------------------------------------------------------------
		// RIGHT PANEL: Search & Add (Width: 480)
		// -------------------------------------------------------------
		Rect{ 800, 0, 480, 720 }.draw(Palette::Whitesmoke);

		// Search Header
		FontAsset(U"Header")(U"Add New Course").draw(820, 20, Palette::Black);

		// Input Area
		SimpleGUI::TextBox(codeInput, Vec2{ 820, 70 }, 200);

		if (SimpleGUI::Button(U"Search", Vec2{ 1030, 70 }, 100) && !isFetching)
		{
			if (appConfig.valid && !codeInput.text.isEmpty()) {
				fetcher.startFetch(codeInput.text, U"2025", appConfig);
				isFetching = true;
				searchResult = none; // Clear old result
				statusMessage = U"Searching...";
			}
		}

		// Search Status
		if (isFetching)
		{
			Circle{ 1040, 150, 20 }.drawArc(Scene::Time() * 180_deg, 270_deg, 4, 4, Palette::Skyblue);

			if (fetcher.hasResult())
			{
				isFetching = false;
				SyllabusData res = fetcher.getResult();
				if (res.isValid) {
					searchResult = res;
					statusMessage = U"Found!";
				}
				else {
					statusMessage = U"Course not found.";
				}
			}
		}

		FontAsset(U"Normal")(statusMessage).draw(820, 110, Palette::Gray);

		// Search Result Card
		if (searchResult)
		{
			int ry = 160;
			Rect resCard{ 820, ry, 440, 200 };
			resCard.draw(Palette::White).drawFrame(2, Palette::Orange);

			FontAsset(U"Title")(searchResult->title).draw(830, ry + 10, Palette::Black);
			FontAsset(U"Normal")(searchResult->instructor).draw(830, ry + 40, Palette::Black);
			FontAsset(U"Small")(searchResult->schedule).draw(830, ry + 70, Palette::Black);

			// "ADD TO SCHEDULE" Button
			if (SimpleGUI::Button(U"Add to My Schedule", Vec2{ 830, (double)ry + 140 }, 200))
			{
				// Check for duplicates
				bool exists = mySchedule.includes_if([&](const SyllabusData& c) { return c.code == searchResult->code; });

				if (!exists) {
					mySchedule << searchResult.value();
					SyllabusFetcher::SaveSchedule(mySchedule); // Save immediately
					statusMessage = U"Added to schedule!";
					searchResult = none; // Clear result after adding
				}
				else {
					statusMessage = U"Already in schedule.";
				}
			}
		}
	}
}
