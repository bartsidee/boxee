class IOException {
	public:
		IOException(const char *_reason) : reason(_reason) {};
		virtual ~IOException() {};
		virtual const char *what() { return reason; }
	protected:
		const char *reason;
};

void readToWord(const char *word, istream &in) throw (IOException) {
	int length = strlen(word);
	int pos = 0;
	while (!in.eof()) {
		char c;
		while (in.get(c) && c==word[pos]) {
			if (++pos == length)
				return;
		}
		pos = 0;
	}
	throw IOException("end of file");
}
void readWhiteSpace(istream &in) throw (IOException) {
	while (!in.eof()) {
		char c;
		in.get(c);
		if (!isspace(c)) {
			in.putback(c);
			return;
		}
	}
	throw IOException("end of file");
}
string readTo(istream &in, char c) {
	string temp;
	while (!in.eof() && in.peek()!=c) {
		char c;
		in.get(c);
		temp += c;
	}
	return temp;
}
string readTo(istream &in, const char *word) {
	string temp;

	int length = strlen(word);
	int pos = 0;
	while (true) {
		char c;
		while (in.get(c) && c==word[pos]) {
			if (++pos == length)
				return temp;
		}
		for (int i=0; i<pos; i++)
			temp += word[i];
		if (in.eof())
			return temp;
		temp += c;
		pos = 0;
	}
	return temp;
}
string readWordTo(istream &in, const char *word) {
	string s = readTo(in, word);
	istringstream iss(s);
	string t;
	iss >> t;
	return t;
}
