#include "SyllabusFetcher.hpp"

// ---------------------------------------------------------
// Helper: Parser Logic (Internal)
// ---------------------------------------------------------

ClassDay ParseDay(const String& dayStr)
{
	if (dayStr.includes(U"月")) return ClassDay::Mon;
	if (dayStr.includes(U"火")) return ClassDay::Tue;
	if (dayStr.includes(U"水")) return ClassDay::Wed;
	if (dayStr.includes(U"木")) return ClassDay::Thu;
	if (dayStr.includes(U"金")) return ClassDay::Fri;
	if (dayStr.includes(U"土")) return ClassDay::Sat;
	return ClassDay::Unknown;
}

std::pair<ClassDay, int> ParseSinglePeriod(String s)
{
	s = s.replaced(U"／", U"/").replaced(U"　", U" ").replaced(U"：", U":");

	ClassDay day = ParseDay(s);

	int period = -1;
	for (const auto& ch : s)
	{
		if (IsDigit(ch))
		{
			period = ch - U'0';
			break;
		}
	}

	return { day, period };
}

// ---------------------------------------------------------
// Main Parser Implementation
// ---------------------------------------------------------

Array<ClassSession> SyllabusFetcher::ParseSchedule(const String& scheduleRaw, const String& roomRaw)
{
	Array<ClassSession> sessions;

	const auto timeParts = scheduleRaw.split(U',');
	const auto roomParts = roomRaw.split(U',');

	for (size_t i = 0; i < timeParts.size(); ++i)
	{
		ClassSession session;
		String tStr = timeParts[i].trimmed();

		auto [d, p] = ParseSinglePeriod(tStr);

		// Check against ClassDay::Unknown
		if (d != ClassDay::Unknown && p != -1)
		{
			session.day = d;
			session.period = p;

			String searchKey = U"";
			switch (d) {
			case ClassDay::Mon: searchKey = U"月"; break;
			case ClassDay::Tue: searchKey = U"火"; break;
			case ClassDay::Wed: searchKey = U"水"; break;
			case ClassDay::Thu: searchKey = U"木"; break;
			case ClassDay::Fri: searchKey = U"金"; break;
			case ClassDay::Sat: searchKey = U"土"; break;
			default: break;
			}
			searchKey += Format(p);

			bool roomFound = false;

			for (const auto& rPart : roomParts)
			{
				String rClean = rPart.replaced(U" ", U"").replaced(U"　", U"");

				if (rClean.starts_with(searchKey))
				{
					String rawContent = rPart;
					size_t colonPos = rawContent.indexOf(U':');
					if (colonPos == String::npos) colonPos = rawContent.indexOf(U'：');

					if (colonPos != String::npos)
					{
						String content = rawContent.substr(colonPos + 1).trimmed();
						size_t slashPos = content.indexOf(U'／');
						if (slashPos != String::npos)
						{
							session.classroom = content.substr(slashPos + 1).trimmed();
						}
						else
						{
							session.classroom = content;
						}
					}
					roomFound = true;
					break;
				}
			}

			if (!roomFound && i < roomParts.size() && roomParts.size() == timeParts.size())
			{
				String rPart = roomParts[i];
				size_t slashPos = rPart.lastIndexOf(U'／');
				if (slashPos != String::npos)
					session.classroom = rPart.substr(slashPos + 1).trimmed();
				else
					session.classroom = rPart.trimmed();
			}

			sessions << session;
		}
	}

	return sessions;
}


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
	result.code = targetCode;

	// Construct URL
	const URL requestUrl = U"{}?code={}&year={}&key={}"_fmt(config.vpsUrl, targetCode, targetYear, config.apiKey);

	MemoryWriter writer;

	// Download JSON
	if (SimpleHTTP::Get(requestUrl, {}, writer))
	{
		if (writer.size() == 0) return result;

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

			// Raw Strings
			if (data.hasElement(U"曜限／Period")) result.schedule = data[U"曜限／Period"].getString();
			if (data.hasElement(U"教室／Classroom")) result.classroom = data[U"教室／Classroom"].getString();

			// Parse
			result.sessions = ParseSchedule(result.schedule, result.classroom);

			for (const auto& object : data)
			{
				result.details[object.key] = object.value.getString();
			}
		}
		else
		{
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
