#pragma once
#include <Siv3D.hpp>
#include <future>

struct AppConfig
{
	String vpsUrl;
	String apiKey;
	bool valid = false;
};

struct SyllabusData
{
	bool isValid = false;

	// Core fields for the list view
	String title;
	String instructor;
	String semester;
	String schedule;
	String classroom;

	// Full Dataset: Stores EVERY field returned by the server
	HashTable<String, String> details;
};

class SyllabusFetcher
{
public:
	void startFetch(const String& code, const String& year, const AppConfig& config);
	bool hasResult();
	SyllabusData getResult();

private:
	std::future<SyllabusData> m_future;
	static SyllabusData FetchTask(String code, String year, AppConfig config);
};
