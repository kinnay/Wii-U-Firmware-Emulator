
class LogWriter:
	def __init__(self, name=""):
		self.name = name
		self.buffer = ""
		
	def write(self, data):
		self.buffer += data
		for line in self.buffer.split("\n")[:-1]:
			self.print("%s> %s" %(self.name, line))
		self.buffer = self.buffer.split("\n")[-1]
		
	def close(self):
		if self.buffer:
			self.print("%s> %s" %(self.name, self.buffer))
			
	def print(self, data): raise NotImplementedError("LogWriter.print")

	
class ConsoleLogger(LogWriter):
	def print(self, data):
		print(data)
		
		
class PrintLogger(ConsoleLogger):
	def write(self, data):
		if not data.endswith("\n"):
			data += "\n"
		super().write(data)
		
		
class FileLogger(LogWriter):
	def __init__(self, filename, name=""):
		super().__init__(name)
		self.file = open(filename, "w")
		
	def close(self):
		super().close()
		self.file.close()
		
	def print(self, data):
		self.file.write(data + "\n")
