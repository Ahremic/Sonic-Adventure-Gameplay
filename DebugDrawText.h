#pragma once

class DebugDrawText
{
public:
    struct Location
    {
        size_t x;
        size_t y;
    };

    struct Color
    {
        float r;
        float g;
        float b;
        float a;
    };

protected:
    virtual ~DebugDrawText() = default;
    virtual void drawImpl(const char* text, const Location& location, float size, const Color& color) = 0;
    virtual void logImpl(const char* text, float time, size_t priority, const Color& color) = 0;

private:
    static DebugDrawText* getInstanceImpl();

public:
    static DebugDrawText* getInstance()
    {
#ifdef _USE_LOG
        static DebugDrawText* instance;
        return instance != nullptr ? instance : instance = getInstanceImpl();
#else
		return nullptr;
#endif
    }

    static void draw(const char* text, const Location& location, const float size, const Color& color)
    {
#ifdef _USE_LOG
        getInstance()->drawImpl(text, location, size, color);
#endif
    }

    static void draw(const char* text, const Location& location, const float size)
    {
#ifdef _USE_LOG
        getInstance()->drawImpl(text, location, size, {1, 1, 1, 1});
#endif
    }

    static void draw(const char* text, const Location& location)
    {
#ifdef _USE_LOG
        getInstance()->drawImpl(text, location, 1, {1, 1, 1, 1});
#endif
    }

    static void log(const char* text, const float time, const size_t priority, const Color& color)
    {
#ifdef _USE_LOG
        getInstance()->logImpl(text, time, priority + 5, color);
#endif
    }

    static void log(const char* text, const float time, const size_t priority)
    {
#ifdef _USE_LOG
        getInstance()->logImpl(text, time, priority + 5, {1, 1, 1, 1});
#endif
    }

    static void log(const char* text, const float time)
    {
#ifdef _USE_LOG
        getInstance()->logImpl(text, time, 0 + 5, {1, 1, 1, 1});
#endif
    }

    static void log(const char* text)
    {
#ifdef _USE_LOG
        getInstance()->logImpl(text, 0.0f, 0 + 5, {1, 1, 1, 1});
#endif
    }
};

class DebugDrawTextNullImpl : public DebugDrawText
{
protected:
    void drawImpl(const char* text, const Location& location, float size, const Color& color) override {}
    void logImpl(const char* text, float time, size_t priority, const Color& color) override {}
};

// Define this before including the .h file in your project
#ifdef _USE_LOG

inline DebugDrawText* DebugDrawText::getInstanceImpl()
{
    const HMODULE module = GetModuleHandle(TEXT("GenerationsParameterEditor.dll"));

    FARPROC procAddress;
    if (module == nullptr || (procAddress = GetProcAddress(module, "DebugDrawTextGetInstance")) == nullptr)
        return new DebugDrawTextNullImpl();

    return ((DebugDrawText*(*)())procAddress)();
}

// Use this function only for directly passing formatted strings to draw/log functions
inline char* format(const char* fmt, ...)
{
    thread_local static char buffer[1024];

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    return buffer;
}

#else
	
inline DebugDrawText* DebugDrawText::getInstanceImpl()
{
	return nullptr;
}

inline char* format(const char* fmt, ...)
{
    return nullptr;
}

#endif