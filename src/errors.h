
void IOError(const char *message, ...);
void KeyError(const char *message, ...);
void MemoryError(const char *message, ...);
void NotImplementedError(const char *message, ...);
void OverflowError(const char *message, ...);
void RuntimeError(const char *message, ...);
void TypeError(const char *message, ...);
void ValueError(const char *message, ...);

void ParseError(const char *message, ...);

bool ErrorOccurred();

void Errors_Init();
