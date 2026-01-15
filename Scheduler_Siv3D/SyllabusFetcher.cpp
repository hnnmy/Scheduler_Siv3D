#include "SyllabusFetcher.hpp"

// ---------------------------------------------------------
// Class Implementation
// ---------------------------------------------------------

void SyllabusFetcher::startFetch(const String& code, const String& year, const AppConfig& config)
{
	m_future = std::async(std::launch::async, [=]() {
		return FetchTask(code, year, config);
	});
}

bool SyllabusFetcher::hasResult()
{
	return m_future.valid() && (m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
}

SyllabusData SyllabusFetcher::getResult()
{
	if (hasResult()) return m_future.get();
	return SyllabusData{};
}

SyllabusData SyllabusFetcher::FetchTask(String targetCode, String targetYear, AppConfig config)
{
	SyllabusData result;

	// Construct URL
	const URL requestUrl = U"{}?code={}&year={}&key={}"_fmt(config.vpsUrl, targetCode, targetYear, config.apiKey);

	MemoryWriter writer;

	// Download JSON
	if (SimpleHTTP::Get(requestUrl, {}, writer))
	{
		if (writer.size() == 0) return result;

		// [FIX] Use MemoryReader instead of MemoryViewReader
		// This takes ownership of the data blob, preventing the "initializer list" build error.
		const JSON json = JSON::Load(MemoryReader{ writer.getBlob() });

		if (json && json[U"found"].getOr<bool>(false))
		{
			result.isValid = true;
			const JSON& data = json[U"data"];

			// 1. Map Core Fields (Must match Python keys exactly)
			// Using .getString() is safe here because we know 'data' exists if 'found' is true
			if (data.hasElement(U"科目名／Course title")) result.title = data[U"科目名／Course title"].getString();
			if (data.hasElement(U"主担当教員名／Instructor")) result.instructor = data[U"主担当教員名／Instructor"].getString();
			if (data.hasElement(U"学期／Semester")) result.semester = data[U"学期／Semester"].getString();
			if (data.hasElement(U"曜限／Period")) result.schedule = data[U"曜限／Period"].getString();
			if (data.hasElement(U"教室／Classroom")) result.classroom = data[U"教室／Classroom"].getString();

			// 2. Store All Fields
			for (const auto& object : data)
			{
				result.details[object.key] = object.value.getString();
			}
		}
		else
		{
			// [FIX] Safe Error Checking
			// Prevents "Key 'error' not found" runtime crash
			String err = U"Unknown Error";
			if (json.hasElement(U"error")) err = json[U"error"].getString();
			else if (json.hasElement(U"message")) err = json[U"message"].getString();

			Console << U"[API Error] " << err;
		}
	}
	else
	{
		Console << U"[Network Error] Could not reach server.";
	}

	return result;
}
