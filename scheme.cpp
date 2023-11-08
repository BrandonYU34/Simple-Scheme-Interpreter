# include <iostream>
# include <map>
# include <string>
# include <vector>
# include <sstream>
# include <exception>
# include <iomanip>
# include <cctype>
# include <cstdlib>
# include <cstring>
# include <cstdio>

using namespace std;


int gline = 1, gcol = 0;
int gtestnum;
bool gOutputDone = false, gTerminate = false;

void LCReset(int x, int y) {
	gcol = y;
	gline = x;
}

char Getchar() {
	char c = getchar();
	gcol++;
	if (c == EOF)
		return '\r';
	else
		return c;
}

void Putback(char c) {
	cin.putback(c);
	--gcol;
}

string To_string(int i) {
	stringstream ss;
	ss << i;
	return ss.str();
}

string gsyscmd[] =
{
	"exit", "cons", "quote", "list", "define","car", "cdr","atom?",
	"pair?", "list?", "null?", "integer?", "real?", "number?",
	"string?", "boolean?", "symbol?", "+", "-", "*", "/", "not",
	"and", "or", ">", ">=", "<", "<=", "=", "string-append",
	"string>?", "string<?", "string=?", "eqv?", "equal?", "begin",
	"if", "cond", "clean-environment", "lambda", "let"
};


enum TokenType {
	EXIT, CONS, QUOTE, LIST, DEFINE, CAR, CDR, ISATOM, ISPAIR,
	ISLIST, ISNULL, ISINT, ISREAL, ISNUM, ISSTR, ISBOOL, ISSYM,
	ADD, SUB, MUL, DIV, NOT, AND, OR, GT, GE, LT, LE, EQ, STRA,
	STRGT, STRLT, STREQ, ISEQV, ISEQU, BEGIN, IF, COND, CLEANENV,
	LAMBDA, LET, EMPTY, NIL, LPAR, RPAR, DOT, INT, FLOAT, STRING,
	SHARP, OPERATOR, SYMBOL, T
};

enum ExceptionType {
	ERROR0, EOFOCCUR, EXPATOM, EXPRP, EOLINSTR, NONLIST, INCNARG,
	UNBOUNDPAR, NOREV, ERRLEVEL, FORERR, UNBOUNDSYM, INCARGT,
	DIVZERO, UNBOUNDTC, NONFUN, WNOREV
};



class Token;
class Function;
typedef Token* TokenPtr;
typedef vector<TokenPtr> VOT;
typedef map<string, TokenPtr> DMAP;
DMAP globalDefine, goriDefine;

class Token {
public:
	int mtype;
	string mcontent;
	TokenPtr mleft, mright;
	bool minquote;
	Function* mfunction;
	Token() {
		mtype = EMPTY;
		mcontent = "";
		mleft = NULL;
		mright = NULL;
		minquote = false;
	} // Token()

	Token(int t, string c) {
		mtype = t;
		mcontent = c;
		mleft = NULL;
		mright = NULL;
		minquote = false;
	} // Token()

	Token(int t, string c, TokenPtr l, TokenPtr r) {
		mtype = t;
		mcontent = c;
		mleft = l;
		mright = r;
		minquote = false;
	} // Token()

	void Set(int t, string c) {
		mtype = t;
		mcontent = c;
	}


};


class Exception {
public:
	int mtype;
	string merrmsg;
	Exception() {
		mtype = ERROR0;
		merrmsg = "ERROR0";
	}

	Exception(int emtype) {
		mtype = emtype;
		if (mtype == EOFOCCUR) {
			merrmsg = "ERROR (no more input) : END-OF-FILE encountered";
			gTerminate = true;
		}
		else if (mtype == EOLINSTR) {
			merrmsg = "ERROR (no closing quote) : END-OF-LINE encountered at Line " +
				To_string(gline) + " Column " + To_string(gcol);
		}
		else if (mtype == DIVZERO)
			merrmsg = "ERROR (division by zero) : /";

		LCReset(1, 0);
	}

	Exception(TokenPtr t, int emtype, char errChar) {
		mtype = emtype;
		if (mtype == EXPATOM) {
			if (t->mcontent == ")")
				gcol++;
			merrmsg = "ERROR (unexpected token) : atom or '(' expected when token at Line "
				+ To_string(gline) + " Column " + To_string(gcol - t->mcontent.size())
				+ " is >>" + t->mcontent + "<<";
			while (errChar != '\n' && errChar != '\r')
				errChar = Getchar();
			LCReset(1, 0);
		}
		else if (mtype == EXPRP) {
			merrmsg = "ERROR (unexpected token) : \')\' expected when token at Line "
				+ To_string(gline) + " Column " + To_string(gcol - t->mcontent.size() + 1)
				+ " is >>" + t->mcontent + "<<";
			while (errChar != '\n' && errChar != '\r')
				errChar = Getchar();
			LCReset(1, 0);
		}

	}

	Exception(int emtype, string s) {
		mtype = emtype;
		if (mtype == NONLIST)
			merrmsg = "ERROR (non-list) : " + s;
		else if (mtype == INCNARG)
			merrmsg = "ERROR (incorrect number of arguments) : " + s;
		else if (mtype == UNBOUNDPAR)
			merrmsg = "ERROR (unbound parameter) : " + s;
		else if (mtype == NOREV || mtype == WNOREV)
			merrmsg = "ERROR (no return value) : " + s;
		else if (mtype == ERRLEVEL)
			merrmsg = "ERROR (level of " + s + ")";
		else if (mtype == UNBOUNDSYM)
			merrmsg = "ERROR (unbound symbol) : " + s;
		else if (mtype == UNBOUNDTC)
			merrmsg = "ERROR (unbound test-condition) : " + s;
		else if (mtype == NONFUN)
			merrmsg = "ERROR (attempt to apply non-function) : " + s;


		LCReset(1, 0);
	}

	Exception(int emtype, string cmdToken, string s) {
		mtype = emtype;
		if (mtype == INCARGT)
			merrmsg = "ERROR (" + cmdToken + " with incorrect argument type) : " + s;
		else if (mtype == FORERR)
			merrmsg = "ERROR (" + cmdToken + " format) : " + s;

		LCReset(1, 0);
	}



};

class Function {
public:
	VOT margs;
	VOT mexecutions;

	Function() {}
	TokenPtr Eval(VOT& arguements, int level, string fname);
};

int IsSysCmd(TokenPtr t) {
	string s = gsyscmd[0];
	int cmdnum = sizeof(gsyscmd) / sizeof(s);
	if (t->mtype == LAMBDA && t->mcontent != gsyscmd[LAMBDA])
		return -1;
	else if (t->mtype < cmdnum && !t->minquote)
		return t->mtype;
	for (int i = 0; i < cmdnum; i++) {
		if (!t->minquote && t->mcontent == gsyscmd[i]) {
			t->mtype = i;
			return i;
		}
	}

	return -1;
} // IsSysCmd()


TokenPtr GetToken(bool nextDOT, bool nextRPAR) {
	TokenPtr reToken = new Token();
	bool tokenEnd = false;
	char inputC;
	while (!tokenEnd) {
		inputC = Getchar();
		if (reToken->mtype == EMPTY) {
			if (inputC == '(' || inputC == ')') {
				if (inputC == '(')
					reToken->mtype = LPAR;
				else
					reToken->mtype = RPAR;
				tokenEnd = true;
			} // if ()
			else if (inputC == '+' || inputC == '-')
				reToken->mtype = OPERATOR;
			else if (inputC == '#')
				reToken->mtype = SHARP;
			else if (inputC == '\"')
				reToken->mtype = STRING;
			else if (inputC == '\'') {
				reToken->mtype = QUOTE;
				tokenEnd = true;
			} // else if ()
			else if (inputC == '.')
				reToken->mtype = DOT;
			else if (inputC == '\"')
				reToken->mtype = STRING;
			else if (isdigit(inputC))
				reToken->mtype = INT;
			else if (isalpha(inputC)) {
				if (inputC == 't')
					reToken->mtype = T;
				else
					reToken->mtype = SYMBOL;
			}
			else if (inputC == '\n') {
				if (gOutputDone) {
					gOutputDone = !gOutputDone;
					gline = 1;
				}
				else
					gline++;
				gcol = 0;
			}
			else if (inputC == ';') {
				inputC = Getchar();
				while (inputC != '\r' && inputC != '\n')
					inputC = Getchar();
				if (inputC == '\r')
					throw Exception(EOFOCCUR);
				else
					Putback(inputC);
			}
			else if (inputC == '\r')
				throw Exception(EOFOCCUR);
			else if (!isspace(inputC))
				reToken->mtype = SYMBOL;

			if (inputC != '\r' && !isspace(inputC) && inputC != ';')
				reToken->mcontent += inputC;
		}
		else if (reToken->mtype == STRING) {
			if (inputC == '\"') {
				tokenEnd = true;
				reToken->mcontent += inputC;
			}
			else {
				if (inputC == '\n')
					throw Exception(EOLINSTR);
				else if (inputC == '\r') {
					throw Exception(EOFOCCUR);
					gTerminate = true;
				}
				else if (inputC == '\\') {
					char nextC = Getchar();
					if (nextC == '\r') {
						throw Exception(EOFOCCUR);
						gTerminate = true;
					}
					else if (nextC == 'n' || nextC == 't' || nextC == '\"' || nextC == '\\') {
						if (nextC == 'n')
							inputC = '\n';
						if (nextC == 't')
							inputC = '\t';
						if (nextC == '\"')
							inputC = '\"';

						reToken->mcontent += inputC;
					}
					else
						reToken->mcontent = reToken->mcontent + inputC + nextC;

				}
				else
					reToken->mcontent += inputC;
			}
		}
		else if (reToken->mtype == DOT) {
			if (inputC == '(' || inputC == ')' || inputC == '\''
				|| inputC == '\"' || isspace(inputC) || inputC == ';') {
				if (!nextDOT)
					throw Exception(reToken, EXPATOM, inputC);
				else {
					tokenEnd = true;
					Putback(inputC);
				}
			}
			else {
				if (isdigit(inputC))
					reToken->mtype = FLOAT;
				else
					reToken->mtype = SYMBOL;
				reToken->mcontent += inputC;
			}
		}
		else if (reToken->mtype == INT) {
			if (inputC == '(' || inputC == ')' || inputC == '\''
				|| inputC == '\"' || isspace(inputC) || inputC == ';') {
				tokenEnd = true;
				Putback(inputC);
			}
			else {
				if (inputC == '.')
					reToken->mtype = FLOAT;
				else if (!isdigit(inputC))
					reToken->mtype = SYMBOL;
				reToken->mcontent += inputC;
			}
		}
		else if (reToken->mtype == FLOAT) {
			if (inputC == '(' || inputC == ')' || inputC == '\''
				|| inputC == '\"' || isspace(inputC) || inputC == ';') {
				tokenEnd = true;
				Putback(inputC);
			}
			else {
				if (!isdigit(inputC))
					reToken->mtype = SYMBOL;
				reToken->mcontent += inputC;
			}
		}
		else if (reToken->mtype == SHARP) {
			if (inputC == '(' || inputC == ')' || inputC == '\''
				|| inputC == '\"' || isspace(inputC) || inputC == ';') {
				tokenEnd = true;
				Putback(inputC);
				reToken->mtype = SYMBOL;
			}
			else {
				if (inputC == '.' || isdigit(inputC))
					reToken->mtype = SYMBOL;
				else if (isalpha(inputC)) {
					if (inputC == 't')
						reToken->mtype = T;
					else if (inputC == 'f')
						reToken->mtype = NIL;
					else
						reToken->mtype = SYMBOL;
				}
				else
					reToken->mtype = SYMBOL;
				reToken->mcontent += inputC;
			}
		}
		else if (reToken->mtype == OPERATOR) {
			if (inputC == '(' || inputC == ')' || inputC == '\''
				|| inputC == '\"' || isspace(inputC) || inputC == ';') {
				tokenEnd = true;
				Putback(inputC);
				reToken->mtype = SYMBOL;
			}
			else {
				if (inputC == '.')
					reToken->mtype = FLOAT;
				else if (isdigit(inputC))
					reToken->mtype = INT;
				else // include isalpha(inputC)
					reToken->mtype = SYMBOL;
				reToken->mcontent += inputC;
			}
		}
		else if (reToken->mtype == NIL || reToken->mtype == T || reToken->mtype == SYMBOL) {
			if (inputC == '(' || inputC == ')' || inputC == '\''
				|| inputC == '\"' || isspace(inputC) || inputC == ';') {
				tokenEnd = true;
				Putback(inputC);
			}
			else {
				reToken->mtype = SYMBOL;
				reToken->mcontent += inputC;
			}
		}


	}
	if ((!nextDOT && reToken->mtype == DOT) ||
		(!nextRPAR && reToken->mtype == RPAR))
		throw Exception(reToken, EXPATOM, inputC);

	return reToken;
}

void ReadSExp(TokenPtr head);
void QuoteExpProcess(TokenPtr head) {
	head->Set(LPAR, "(");
	head->mleft = new Token(QUOTE, "quote");
	head->mright = new Token(LPAR, ".(");
	head->mright->mleft = GetToken(false, false);
	if (head->mright->mleft->mtype == QUOTE || head->mright->mleft->mtype == LPAR)
		ReadSExp(head->mright->mleft);
}

void ReadSExp(TokenPtr head) {
	bool rpencounter = true;
	if (head->mtype == QUOTE) {
		rpencounter = false;
		QuoteExpProcess(head);
	}

	if (head->mleft == NULL) {
		head->mleft = GetToken(false, true);
		if (head->mleft->mtype == LPAR)
			ReadSExp(head->mleft);
		else if (head->mleft->mtype == QUOTE)
			QuoteExpProcess(head->mleft);
		else if (head->mleft->mtype == RPAR) {
			head->Set(NIL, "nil");
			head->mleft = NULL;
			return;
		}
	}
	if (head->mright == NULL) {
		head->mright = GetToken(true, true);
		if (head->mright->mtype == QUOTE) {
			rpencounter = false;
			head->mright = new Token(LPAR, ".(");
			head->mright->mleft = new Token(QUOTE, "quote");
			QuoteExpProcess(head->mright->mleft);
			ReadSExp(head->mright);
		}
		else if (head->mright->mtype == DOT) {
			head->mright = GetToken(false, false);
			if (head->mright->mtype == LPAR) {
				head->mright->mcontent = ".(";
				ReadSExp(head->mright);
			}
			else if (head->mright->mtype == QUOTE) {
				rpencounter = false;
				head->mright = new Token(LPAR, ".(");
				head->mright->mleft = new Token(QUOTE, "quote");
				ReadSExp(head->mright);
			}
		}
		else if (head->mright->mtype != RPAR) {
			rpencounter = false;
			TokenPtr temp = head->mright;
			head->mright = new Token(LPAR, ".(");
			head->mright->mleft = temp;
			if (temp->mtype == LPAR)
				ReadSExp(temp);
			ReadSExp(head->mright);
		}
	}

	if (rpencounter && head->mright->mtype != RPAR) {
		TokenPtr temp = GetToken(true, true);
		if (temp->mtype != RPAR)
			throw Exception(temp, EXPRP, '\0');
		else
			return;
	}

}

void AdjustContent(TokenPtr t) {
	if (t->mtype == T)
		t->mcontent = "#t";
	else if (t->mtype == NIL)
		t->mcontent = "nil";
	else if (t->mtype == SYMBOL) {
		if (t->mcontent == "nil")
			t->mtype = NIL;
		else if (t->mcontent == "quote")
			t->mtype = QUOTE;
	}
	else if (t->mtype == INT) {
		stringstream ss;
		ss << atoi(t->mcontent.c_str());
		t->mcontent = ss.str();
	}
	else if (t->mtype == FLOAT) {
		if (t->mcontent == "+." || t->mcontent == "-.")
			t->mtype = SYMBOL;
		else {
			stringstream ss;
			ss << fixed << setprecision(3) << atof(t->mcontent.c_str());
			t->mcontent = ss.str();
		}
	}
	else if (t->mtype == RPAR)
		t->Set(NIL, "nil");
	else if (t->mleft != NULL && t->mright == NULL)
		t->mright = new Token(NIL, "nil");
}

void AdjustExp(TokenPtr head) {
	AdjustContent(head);
	if (head->mleft != NULL)
		AdjustExp(head->mleft);
	if (head->mright != NULL)
		AdjustExp(head->mright);
}

string To_procedure(TokenPtr t, bool inErr) {
	if (t->mtype == LAMBDA && t->mcontent != "")
		return "#<procedure " + t->mcontent + ">";
	else if (inErr || (IsSysCmd(t) == -1 && t->mtype != LAMBDA))
		return t->mcontent;
	else
		return "#<procedure " + gsyscmd[t->mtype] + ">";
}

string PrintSExp(TokenPtr head, int col, bool inErr, string output) {
	if (head == NULL)
		return "";
	if (head->mtype == LPAR) {
		output += "( ";
		if (head->mleft != NULL)
			output = PrintSExp(head->mleft, col + 1, inErr, output);
		if (head->mright != NULL && head->mright->mtype != RPAR) {
			TokenPtr thishead = head->mright;
			if (thishead->mtype == LPAR) {
				for (TokenPtr t = thishead; t != NULL && t->mtype != RPAR; t = t->mright) {
					if (t->mtype == LPAR) {
						if (t->mleft->mtype == LPAR) {
							for (int i = 0; i < col * 2; i++)
								output += ' ';
							output = PrintSExp(t->mleft, col + 1, inErr, output);
						}
						else {
							for (int i = 0; i < col * 2; i++)
								output += ' ';
							output += To_procedure(t->mleft, inErr) + "\n";
						}
					}
					else if (t->mtype != NIL) {
						for (int i = 0; i < col * 2; i++)
							output += ' ';
						output += ".\n";
						for (int i = 0; i < col * 2; i++)
							output += ' ';
						output += To_procedure(t, inErr) + "\n";
					}
				}
			}
			else if (thishead->mtype != NIL) {
				for (int i = 0; i < col * 2; i++)
					output += ' ';
				output += ".\n";
				for (int i = 0; i < col * 2; i++)
					output += ' ';
				output += To_procedure(thishead, inErr) + "\n";
			}
		}
		for (int i = 0; i < (col - 1) * 2; i++)
			output += ' ';
		output += ')';
		if (col > 1)
			output += "\n";
	}
	else {
		output += To_procedure(head, inErr);
		if (col > 1)
			output += "\n";
	}

	return output;
}

TokenPtr EvalSExp(TokenPtr head, int level);

void QuoteToken(TokenPtr t) {
	t->minquote = true;
}

TokenPtr Quote(TokenPtr head, int as) {
	if (as != 1)
		throw Exception(INCNARG, gsyscmd[QUOTE]);

	TokenPtr s = head;
	head->minquote = true;
	if (head->mleft != NULL)
		Quote(head->mleft, as);
	if (head->mright != NULL)
		Quote(head->mright, as);
	return s;
}

TokenPtr Exit(VOT& arguements, int level) {
	if (level > 0)
		throw Exception(ERRLEVEL, "EXIT");
	else if (arguements.size() != 0)
		throw Exception(INCNARG, "exit");

	cout << "\nThanks for using OurScheme!";
	exit(0);
	return NULL;
}

TokenPtr Cons(VOT& arguements, int level) {
	if (arguements.size() != 2)
		throw Exception(INCNARG, gsyscmd[CONS]);

	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
		}
		catch (Exception error) {
			if (error.mtype == NOREV) {
				string s = PrintSExp(arguements[i], 1, true, "");
				throw Exception(UNBOUNDPAR, s);
			}
			else
				throw;
		}
	}

	return new Token(LPAR, "(", arguements[0], arguements[1]);
}

TokenPtr List(VOT& arguements, int level) {
	if (arguements.size() == 0)
		return new Token(NIL, "nil");

	TokenPtr res = new Token(LPAR, "(", EvalSExp(arguements[0], level + 1), NULL);
	TokenPtr t = res;
	for (int i = 1; i < arguements.size(); i++, t = t->mright) {
		try {
			t->mright = new Token(LPAR, ".(", EvalSExp(arguements[i], level + 1), NULL);
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}
	}

	t = new Token(NIL, "nil");
	return res;
}

TokenPtr Lambda(TokenPtr head, VOT& arguements, int level, string opType) {
	if (arguements.size() < 2)
		throw Exception(FORERR, opType, PrintSExp(head, 1, true, ""));
	else if (arguements[0]->mtype != NIL && arguements[0]->mtype != LPAR)
		throw Exception(FORERR, opType, PrintSExp(head, 1, true, ""));

	VOT selfArg;
	TokenPtr res = new Token(LAMBDA, "");
	if (arguements[0]->mtype != NIL) {
		for (TokenPtr t = arguements[0]; t != NULL; t = t->mright) {
			if (t->mtype != NIL) {
				if (t->mright == NULL)
					throw Exception(NONLIST, PrintSExp(arguements[0], 1, true, ""));
				else {
					if (t->mleft->mtype != SYMBOL || IsSysCmd(t->mleft) != -1)
						throw Exception(FORERR, opType, PrintSExp(head, 1, true, ""));
					else
						selfArg.push_back(t->mleft);
				}
			}
		}
	}

	res->mfunction = new Function();
	res->mfunction->margs = selfArg;
	for (int i = 1; i < arguements.size(); i++)
		res->mfunction->mexecutions.push_back(arguements[i]);

	return res;
}

TokenPtr Define(TokenPtr head, VOT& arguements, int level) {
	if (level > 0)
		throw Exception(ERRLEVEL, "DEFINE");
	else if (arguements.size() < 2)
		throw Exception(FORERR, "DEFINE", PrintSExp(head, 1, true, ""));
	else if (arguements[0]->minquote || IsSysCmd(arguements[0]) != -1)
		throw Exception(FORERR, "DEFINE", PrintSExp(head, 1, true, ""));
	else if (arguements[0]->mtype != SYMBOL && arguements[0]->mtype != LPAR)
		throw Exception(FORERR, "DEFINE", PrintSExp(head, 1, true, ""));

	TokenPtr def_target = arguements[0];
	if (def_target->mtype == LPAR) {
		if (def_target->mleft->mtype != SYMBOL)
			throw Exception(FORERR, "DEFINE", PrintSExp(head, 1, true, ""));

		def_target = def_target->mleft;
		arguements[0] = arguements[0]->mright;
		TokenPtr res = Lambda(head, arguements, level, "DEFINE");
		res->mcontent = def_target->mcontent;
		globalDefine[def_target->mcontent] = res;

	}
	else {
		if (arguements.size() != 2)
			throw Exception(FORERR, "DEFINE", PrintSExp(head, 1, true, ""));
		if (IsSysCmd(arguements[1]) == -1 && arguements[1]->mtype == SYMBOL
			&& globalDefine.count(arguements[1]->mcontent) == 0)
			throw Exception(UNBOUNDSYM, arguements[1]->mcontent);

		try {
			arguements[1] = EvalSExp(arguements[1], level + 1);
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(WNOREV, PrintSExp(arguements[1], 1, true, ""));
			else
				throw;
		}

		globalDefine[def_target->mcontent] = arguements[1];
	}

	if (head != NULL)
		cout << def_target->mcontent << " defined";

	goriDefine = globalDefine;
	return NULL;
}

TokenPtr CarCdr(VOT& arguements, int level, int tType) {
	if (arguements.size() != 1)
		throw Exception(INCNARG, gsyscmd[tType]);

	try {
		arguements[0] = EvalSExp(arguements[0], level + 1);
	}
	catch (Exception error) {
		if (error.mtype == NOREV)
			throw Exception(UNBOUNDPAR, PrintSExp(arguements[0], 1, true, ""));
		else
			throw;
	}

	if (arguements[0]->mtype != LPAR)
		throw Exception(INCARGT, gsyscmd[tType], To_procedure(arguements[0], false));

	if (tType == CAR)
		return arguements[0]->mleft;
	else
		return arguements[0]->mright;
}

TokenPtr IsX(VOT& arguements, int level, int x) {
	if (arguements.size() != 1)
		throw Exception(INCNARG, gsyscmd[x]);

	try {
		arguements[0] = EvalSExp(arguements[0], level + 1);
	}
	catch (Exception error) {
		if (error.mtype == NOREV)
			throw Exception(UNBOUNDPAR, PrintSExp(arguements[0], 1, true, ""));
		else
			throw;
	}

	if (x == ISPAIR) {
		if (arguements[0]->mtype == LPAR)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}
	else if (x == ISLIST) {
		if (arguements[0]->mtype == NIL)
			return new Token(T, "#t");
		if (arguements[0]->mtype == LPAR) {
			for (TokenPtr temp = arguements[0]; temp != NULL; temp = temp->mright) {
				if (temp->mright == NULL && temp->mtype == NIL)
					return new Token(T, "#t");
			}
		}

		return new Token(NIL, "nil");
	}
	else if (x == ISATOM) {
		if (arguements[0]->mtype != LPAR)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}
	else if (x == ISNULL) {
		if (arguements[0]->mtype == NIL)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}
	else if (x == ISINT) {
		if (arguements[0]->mtype == INT)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}
	else if (x == ISREAL || x == ISNUM) {
		if (arguements[0]->mtype == INT || arguements[0]->mtype == FLOAT)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}
	else if (x == ISSTR) {
		if (arguements[0]->mtype == STRING)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}
	else if (x == ISBOOL) {
		if (arguements[0]->mtype == T || arguements[0]->mtype == NIL)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}
	else if (x == ISSYM) {
		if (arguements[0]->mtype == SYMBOL || arguements[0]->mtype == LAMBDA)
			return new Token(T, "#t");
		else if (IsSysCmd(arguements[0]) != -1)
			return new Token(T, "#t");
		else
			return new Token(NIL, "nil");
	}

	return new Token(NIL, "nil");
}

TokenPtr Operate(VOT& arguements, int level, int opType) {
	if (arguements.size() < 2)
		throw Exception(INCNARG, gsyscmd[opType]);

	int outputType = INT;
	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
			if (arguements[i]->mtype == FLOAT)
				outputType = FLOAT;
			else if (arguements[i]->mtype == LPAR)
				throw Exception(INCARGT, gsyscmd[opType], PrintSExp(arguements[i], 1, false, ""));
			else if (arguements[i]->mtype != INT)
				throw Exception(INCARGT, gsyscmd[opType], To_procedure(arguements[i], false));
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}
	}


	float res = atof(arguements[0]->mcontent.c_str());
	for (int i = 1; i < arguements.size(); i++) {
		float operand = atof(arguements[i]->mcontent.c_str());
		if (opType == ADD)
			res += operand;
		else if (opType == SUB)
			res -= operand;
		else if (opType == MUL)
			res *= operand;
		else if (opType == DIV) {
			if (operand != 0)
				res /= operand;
			else
				throw Exception(DIVZERO);
		}
	}

	stringstream ss;
	if (outputType == FLOAT)
		ss << fixed << setprecision(3) << res;
	else
		ss << (int)res;

	return new Token(outputType, ss.str());
}

TokenPtr Not(VOT& arguements, int level) {
	if (arguements.size() != 1)
		throw Exception(INCNARG, gsyscmd[NOT]);

	try {
		arguements[0] = EvalSExp(arguements[0], level + 1);
	}
	catch (Exception error) {
		if (error.mtype == NOREV)
			throw Exception(UNBOUNDPAR, PrintSExp(arguements[0], 1, true, ""));
		else
			throw;
	}

	if (arguements[0]->mtype == NIL)
		return new Token(T, "#t");
	else
		return new Token(NIL, "nil");
}

TokenPtr Logical_operate(VOT& arguements, int level, int opType) {
	if (arguements.size() < 2)
		throw Exception(INCNARG, gsyscmd[opType]);

	int tnum = 0;
	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}

		if (opType == AND && arguements[i]->mtype == NIL)
			return new Token(NIL, "nil");
		else if (opType == OR && arguements[i]->mtype != NIL)
			return arguements[i];
	}

	if (opType == AND)
		return arguements.back();
	else
		return new Token(NIL, "nil");
}

TokenPtr Comparison_operate(VOT& arguements, int level, int opType) {
	if (arguements.size() < 2)
		throw Exception(INCNARG, gsyscmd[opType]);

	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
			if (arguements[i]->mtype == LPAR)
				throw Exception(INCARGT, gsyscmd[opType], PrintSExp(arguements[i], 1, false, ""));
			else if (arguements[i]->mtype != INT && arguements[i]->mtype != FLOAT)
				throw Exception(INCARGT, gsyscmd[opType], To_procedure(arguements[i], false));
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}
	}

	bool res = true;
	for (int i = 1; i < arguements.size(); i++) {
		float l = atof(arguements[i - 1]->mcontent.c_str());
		float r = atof(arguements[i]->mcontent.c_str());
		if (opType == GT)
			res = (res && (l > r));
		else if (opType == GE)
			res = (res && (l >= r));
		else if (opType == LT)
			res = (res && (l < r));
		else if (opType == LE)
			res = (res && (l <= r));
		else if (opType == EQ)
			res = (res && (l == r));
	}

	if (res)
		return new Token(T, "#t");
	else
		return new Token(NIL, "nil");
}

TokenPtr StrA(VOT& arguements, int level) {
	if (arguements.size() < 2)
		throw Exception(INCNARG, gsyscmd[STRA]);

	string res = "\"\"";
	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
			if (arguements[i]->mtype == LPAR)
				throw Exception(INCARGT, gsyscmd[STRA], PrintSExp(arguements[i], 1, false, ""));
			else if (arguements[i]->mtype != STRING)
				throw Exception(INCARGT, gsyscmd[STRA], To_procedure(arguements[i], false));
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}

		res = res.substr(0, res.length() - 1) + arguements[i]->mcontent.substr(1);
	}

	return new Token(STRING, res);
}


TokenPtr StrCmp(VOT& arguements, int level, int opType) {
	if (arguements.size() < 2)
		throw Exception(INCNARG, gsyscmd[opType]);

	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
			if (arguements[i]->mtype == LPAR)
				throw Exception(INCARGT, gsyscmd[opType], PrintSExp(arguements[i], 1, false, ""));
			else if (arguements[i]->mtype != STRING)
				throw Exception(INCARGT, gsyscmd[opType], To_procedure(arguements[i], false));
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}
	}

	bool res = true;
	for (int i = 1; i < arguements.size(); i++) {
		if (opType == STRGT)
			res = (res && (arguements[i - 1]->mcontent > arguements[i]->mcontent));
		else if (opType == STRLT)
			res = (res && (arguements[i - 1]->mcontent < arguements[i]->mcontent));
		else if (opType == STREQ)
			res = (res && (arguements[i - 1]->mcontent == arguements[i]->mcontent));
	}

	if (res)
		return new Token(T, "#t");
	else
		return new Token(NIL, "nil");
}

TokenPtr IsEqv(VOT& arguements, int level) {
	if (arguements.size() != 2)
		throw Exception(INCNARG, gsyscmd[ISEQV]);

	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}
	}

	if (arguements[0] == arguements[1])
		return new Token(T, "#t");
	else if (arguements[0]->mtype == T && arguements[1]->mtype == T)
		return new Token(T, "#t");
	else if (arguements[0]->mtype == NIL && arguements[1]->mtype == NIL)
		return new Token(T, "#t");
	else if ((arguements[0]->mtype == INT || arguements[0]->mtype == FLOAT) &&
		(arguements[1]->mtype == INT || arguements[1]->mtype == FLOAT) &&
		(arguements[0]->mcontent == arguements[1]->mcontent))
		return new Token(T, "#t");
	else
		return new Token(NIL, "nil");
}

TokenPtr IsEqu(VOT& arguements, int level) {
	if (arguements.size() != 2)
		throw Exception(INCNARG, gsyscmd[ISEQU]);

	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(UNBOUNDPAR, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}
	}

	if (PrintSExp(arguements[0], 1, false, "") == PrintSExp(arguements[1], 1, false, ""))
		return new Token(T, "#t");
	else
		return new Token(NIL, "nil");
}

TokenPtr Begin(VOT& arguements, int level) {
	if (arguements.size() == 0)
		throw Exception(INCNARG, gsyscmd[BEGIN]);

	for (int i = 0; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
		}
		catch (Exception error) {
			if (error.mtype == NOREV && i == arguements.size() - 1)
				throw Exception(NOREV, PrintSExp(arguements[i], 1, true, ""));
			else if (error.mtype != NOREV)
				throw;
		}
	}

	return arguements.back();
}

TokenPtr If(TokenPtr head, VOT& arguements, int level, int opType) {
	if (arguements.size() != 2 && arguements.size() != 3)
		throw Exception(INCNARG, gsyscmd[opType]);

	TokenPtr cond;
	try {
		cond = EvalSExp(arguements[0], level + 1);
	}
	catch (Exception error) {
		if (error.mtype == NOREV)
			throw Exception(UNBOUNDTC, PrintSExp(arguements[0], 1, true, ""));
		else
			throw;
	}

	if (cond->mtype == NIL) {
		if (arguements.size() == 3)
			return EvalSExp(arguements[2], level + 1);
		else
			throw Exception(NOREV, PrintSExp(head, 1, true, ""));
	}
	else
		return EvalSExp(arguements[1], level + 1);
}

TokenPtr Cond(TokenPtr head, VOT& arguements, int level) {
	if (arguements.size() == 0)
		throw Exception(FORERR, "COND", PrintSExp(head, 1, true, ""));

	TokenPtr res = NULL;
	vector<VOT> exec;
	for (int i = 0; i < arguements.size(); i++) {
		try {
			VOT newarg;
			if (arguements[i]->mtype == SYMBOL && globalDefine.count(arguements[i]->mcontent) == 1)
				EvalSExp(arguements[i], level + 1);
			if (arguements[i]->mtype != LPAR || arguements[i]->mleft == NULL)
				throw Exception(FORERR, "COND", PrintSExp(head, 1, true, ""));
			else if (arguements[i]->mright->mright == NULL) {
				if (arguements[i]->mright->mtype == NIL)
					throw Exception(FORERR, "COND", PrintSExp(head, 1, true, ""));
				else {
					string s = PrintSExp(arguements[i], 1, true, "");
					throw Exception(NONLIST, s);
				}
			}

			if (i == arguements.size() - 1 && arguements[i]->mleft->mcontent == "else")
				arguements[i]->mleft->Set(T, "#t");
			newarg.push_back(arguements[i]->mleft);
			for (TokenPtr t = arguements[i]->mright; t != NULL; t = t->mright) {
				if (t->mtype != NIL && t->mright == NULL)
					throw Exception(NONLIST, PrintSExp(arguements[i], 1, true, ""));
				else if (t->mright != NULL)
					newarg.push_back(t->mleft);

			}

			exec.push_back(newarg);
		}
		catch (Exception error) {
			throw;
		}
	}

	for (int i = 0; i < exec.size(); i++) {
		try {
			if (EvalSExp(exec[i][0], level + 1)->mtype != NIL) {
				for (int j = 1; j < exec[i].size(); j++) {
					TokenPtr t = EvalSExp(exec[i][j], level + 1);
					if (j == exec[i].size() - 1)
						return t;
				}
			}
		}
		catch (Exception error) {
			throw;
		}
	}

	string s = PrintSExp(head, 1, true, "");
	throw Exception(NOREV, s);
	return NULL;
}

TokenPtr CleanEnv(VOT& arguements, int level) {
	if (level > 0)
		throw Exception(ERRLEVEL, "CLEAN-ENVIRONMENT");
	else if (arguements.size() != 0)
		throw Exception(INCNARG, gsyscmd[CLEANENV]);

	globalDefine.clear();
	goriDefine.clear();
	cout << "environment cleaned";
	return NULL;
}

TokenPtr Function::Eval(VOT& arguements, int level, string fname) {
	if (margs.size() != arguements.size()) {
		if (fname == "")
			fname = "lambda";
		throw Exception(INCNARG, fname);
	}

	for (int i = 0; i < margs.size(); i++) {
		VOT defarg;
		try {
			arguements[i] = EvalSExp(arguements[i], level + 1);
		}
		catch (Exception error) {
			if (error.mtype == NOREV)
				throw Exception(WNOREV, PrintSExp(arguements[i], 1, true, ""));
			else
				throw;
		}
	}

	DMAP lastMap = globalDefine, locMap = goriDefine;
	for (int i = 0; i < margs.size(); i++)
		locMap[margs[i]->mcontent] = arguements[i];


	TokenPtr res;
	for (int i = 0; i < mexecutions.size(); i++) {
		try {
			globalDefine = locMap;
			res = EvalSExp(mexecutions[i], level);
		}
		catch (Exception error) {
			globalDefine = lastMap;
			if (i == mexecutions.size() - 1 && error.mtype == NOREV)
				throw Exception(NOREV, PrintSExp(mexecutions[i], 1, true, ""));
			else if (error.mtype != NOREV)
				throw;
		}
	}

	globalDefine = lastMap;
	return res;
} // Function::Eval()

TokenPtr Let(TokenPtr head, VOT& arguements, int level) {
	if (arguements.size() < 2)
		throw Exception(FORERR, "LET", PrintSExp(head, 1, true, ""));
	if (arguements[0]->mtype != NIL && arguements[0]->mtype != LPAR)
		throw Exception(FORERR, "LET", PrintSExp(head, 1, true, ""));
	VOT locDefs, executions;
	if (arguements[0]->mtype == LPAR) {
		for (TokenPtr t = arguements[0]; t != NULL; t = t->mright) {
			if (t->mtype != NIL) {
				if (t->mright == NULL)
					throw Exception(NONLIST, PrintSExp(arguements[0], 1, true, ""));
				else {
					if (t->mleft->mtype != LPAR)
						throw Exception(FORERR, "LET", PrintSExp(head, 1, true, ""));
					else
						locDefs.push_back(t->mleft);
				}
			}
		}
	}

	vector<string> defSyms;
	for (int i = 0; i < locDefs.size(); i++) {
		if (locDefs[i]->mleft->mtype != SYMBOL || IsSysCmd(locDefs[i]) != -1)
			throw Exception(FORERR, "LET", PrintSExp(head, 1, true, ""));
		else if (locDefs[i]->mright->mright == NULL) {
			if (locDefs[i]->mright->mtype == NIL)
				throw Exception(FORERR, "LET", PrintSExp(head, 1, true, ""));
			else {
				string s = PrintSExp(locDefs[i], 1, true, "");
				throw Exception(NONLIST, s);
			}
		}
		else {
			defSyms.push_back(locDefs[i]->mleft->mcontent);
			locDefs[i] = locDefs[i]->mright->mleft;
		}

	}

	for (int i = 0; i < locDefs.size(); i++) { 
		try{
			locDefs[i] = EvalSExp(locDefs[i], level);
		}
		catch (Exception error){
			if ( error.mtype == NOREV )
				throw Exception(WNOREV, PrintSExp(locDefs[i], 1, true, ""));
			else
				throw;
		}
	} 
	
	
	DMAP lastMap = globalDefine;
	for (int i = 0; i < locDefs.size(); i++)
		globalDefine[defSyms[i]] = locDefs[i];


	for (int i = 1; i < arguements.size(); i++) {
		try {
			arguements[i] = EvalSExp(arguements[i], level);
		}
		catch (Exception error) {
			globalDefine = lastMap;
			if (error.mtype == NOREV && i == arguements.size() - 1)
				throw Exception(NOREV, PrintSExp(arguements[i], 1, true, ""));
			else if (error.mtype != NOREV)
				throw;
		}
	}

	globalDefine = lastMap;
	return arguements.back();
}


TokenPtr EvalSExp(TokenPtr head, int level) {
	VOT arguements;
	TokenPtr newHead;
	if (head->mcontent == "(") {
		for (TokenPtr t = head->mright; t != NULL; t = t->mright) {
			if (t->mtype != NIL) {
				if (t->mright == NULL)
					throw Exception(NONLIST, PrintSExp(head, 1, true, ""));
				else
					arguements.push_back(t->mleft);
			}
		}

		if (head->mleft->mtype == LPAR) {
			try {
				newHead = EvalSExp(head->mleft, level + 1);
			}
			catch (Exception error) {
				if (error.mtype == NOREV)
					throw Exception(WNOREV, PrintSExp(head->mleft, 1, true, ""));
				else
					throw;
			}
		}
		else
			newHead = EvalSExp(head->mleft, level);

		if (IsSysCmd(newHead) != -1 || newHead->mtype == LAMBDA) {
			try {
				if (newHead->mtype == EXIT)
					return Exit(arguements, level);
				else if (newHead->mtype == CONS)
					return Cons(arguements, level);
				else if (newHead->mtype == QUOTE)
					return Quote(head->mright->mleft, arguements.size());
				else if (newHead->mtype == LIST)
					return List(arguements, level);
				else if (newHead->mtype == DEFINE)
					return Define(head, arguements, level);
				else if (newHead->mtype == CAR || newHead->mtype == CDR)
					return CarCdr(arguements, level, newHead->mtype);
				else if (newHead->mtype >= ISATOM && newHead->mtype <= ISSYM)
					return IsX(arguements, level, newHead->mtype);
				else if (newHead->mtype >= ADD && newHead->mtype <= DIV)
					return Operate(arguements, level, newHead->mtype);
				else if (newHead->mtype == NOT)
					return Not(arguements, level);
				else if (newHead->mtype == AND || newHead->mtype == OR)
					return Logical_operate(arguements, level, newHead->mtype);
				else if (newHead->mtype >= GT && newHead->mtype <= EQ)
					return Comparison_operate(arguements, level, newHead->mtype);
				else if (newHead->mtype == STRA)
					return StrA(arguements, level);
				else if (newHead->mtype >= STRGT && newHead->mtype <= STREQ)
					return StrCmp(arguements, level, newHead->mtype);
				else if (newHead->mtype == ISEQV)
					return IsEqv(arguements, level);
				else if (newHead->mtype == ISEQU)
					return IsEqu(arguements, level);
				else if (newHead->mtype == BEGIN)
					return Begin(arguements, level);
				else if (newHead->mtype == IF)
					return If(head, arguements, level, IF);
				else if (newHead->mtype == COND)
					return Cond(head, arguements, level);
				else if (newHead->mtype == CLEANENV)
					return CleanEnv(arguements, level);
				else if (newHead->mtype == LAMBDA) {
					if (newHead->mcontent == "lambda")
						return Lambda(head, arguements, level, "LAMBDA");
					else
						return newHead->mfunction->Eval(arguements, level, newHead->mcontent);
				}
				else if (newHead->mtype == LET)
					return Let(head, arguements, level);

			}
			catch (Exception error) {
				if (error.mtype == NOREV && level == 0)
					throw Exception(NOREV, PrintSExp(head, 1, true, ""));
				else
					throw;
			}
		}
		else if (newHead->mtype == SYMBOL && !newHead->minquote)
			throw Exception(UNBOUNDSYM, newHead->mcontent);
		else {
			string s = PrintSExp(newHead, 1, false, "");
			throw Exception(NONFUN, s);
		}
	}

	else if (head->mtype == SYMBOL) {
		if (globalDefine.count(head->mcontent) == 1)
			return globalDefine[head->mcontent];
		else if (IsSysCmd(head) != -1)
			return head;
		else
			throw Exception(UNBOUNDSYM, head->mcontent);
	}

	return head;
}



int main() {
	cout << "Welcome to OurScheme!" << endl;
	char c;
	while (!gTerminate) {
		cout << endl << "> ";
		try {
			TokenPtr headToken = GetToken(false, false);
			if (headToken->mtype == QUOTE || headToken->mtype == LPAR) {
				gOutputDone = false;
				ReadSExp(headToken);
			}

			AdjustExp(headToken);
			cout << PrintSExp(EvalSExp(headToken, 0), 1, false, "") << endl;
			gOutputDone = true;
			LCReset(1, 0);
		}
		catch (Exception error) {
			cout << error.merrmsg << endl;
			if (true) {
				char c = Getchar();
				while (c == ' ')
					c = Getchar();
				if (c == ';') {
					while (getchar() != '\n')
						LCReset(1, 0);
				}
				else if (c == '\n')
					LCReset(1, 0);
				else
					Putback(c);
			}
		}
	}

	cout << "Thanks for using OurScheme!";
	return 0;
}

