#pragma once
#include <Siv3D.hpp>
#include <future>

struct AppConfig
{
	String vpsUrl;
	String apiKey;
	bool valid = false;
};

enum class CourseType
{
	Required,   // 必修 (Red)
	Elective,   // 選択 (Blue)
	Other       // その他 (Gray)
};

enum class ClassDay
{
	Mon = 0, Tue, Wed, Thu, Fri, Sat, Unknown = 99
};

// Represents a SINGLE class session (e.g. "Monday Period 4 in Room B108")
// A course like "Mon 4, Wed 5" will have TWO of these in its list.
struct ClassSession
{
	ClassDay day = ClassDay::Unknown;
	int period = 0;        // 0-6 (Standard Sophia periods)
	String classroom;      // e.g. "Kioizaka-B108"
};


struct SyllabusData
{
	bool isValid = false;

	// Core fields for the list view
	String title;
	String instructor;
	String semester;
	String code;
	String schedule;
	String classroom;

	Array<ClassSession> sessions;

	CourseType type = CourseType::Required;
	bool isSelected = false; // True if registered in My Schedule

	// Full Dataset
	HashTable<String, String> details;
};

class SyllabusFetcher
{
public:
	void startFetch(const String& code, const String& year, const AppConfig& config);
	bool hasResult();
	SyllabusData getResult();

	static Array<ClassSession> ParseSchedule(const String& scheduleRaw, const String& roomRaw);

	// Local Storage
	static void SaveSchedule(const Array<SyllabusData>& courses);
	static Array<SyllabusData> LoadSchedule();

private:
	std::future<SyllabusData> m_future;
	static SyllabusData FetchTask(String code, String year, AppConfig config);
};
