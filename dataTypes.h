#define SIZEofBUFF 20
#define SSizeofBUFF 6
#define RECORD_SIZE 300
#define INT_LENGTH 10 /* because INT_MAX has 10 digits */
#define FLOAT_LENGTH 20

typedef enum { false, true } bool;

typedef struct{
	long  	custid;
	char 	FirstName[SIZEofBUFF];
	char 	LastName[SIZEofBUFF];
	char	Street[SIZEofBUFF];
	int 	HouseID;
	char	City[SIZEofBUFF];
	char	postcode[SSizeofBUFF];
	float  	amount;
} Record;


