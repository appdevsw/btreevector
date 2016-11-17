#include <chrono>
#include <sstream>
#include <vector>
#include <assert.h>
#include <locale.h>
#include <BTreeVector.h>

#define BTASIZENODE 16
#define BTASIZELEAF 128

#define dbgprintf if(0)printf

//#define BTA_STRING_TEST

#ifdef BTA_STRING_TEST
#define toVal toValStr
#define BTATYPE std::string
#define printtype "STRING"

#else
#define toVal toValInt
#define BTATYPE int
#define printtype "INT"
#endif

typedef BTreeVector<BTATYPE, BTASIZENODE, BTASIZELEAF> BTAType;

#define dspElapsed(dsp,tstart) \
{\
	auto tstop = std::chrono::system_clock::now();\
	std::chrono::duration<double> elapsed = tstop - tstart;\
	printf("time %s %10.4lf s\n",dsp, elapsed.count());\
}

inline std::string toValStr(int value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

inline int toValInt(int value)
{
    return value;
}

template<class ARR>
void atestspeed(int lmax)
{

    printf("\nspeed test for %'d elements\n", lmax);

    ARR bta;
    BTATYPE eval;

    //--------------
    auto tstart1 = std::chrono::system_clock::now();

    for (int i = 0; i < lmax; i++)
    {
        eval = toVal(i * 2);
        bta.add(eval);
    }

    dspElapsed("append               ", tstart1);

    //---------------
    bta.clear();

    tstart1 = std::chrono::system_clock::now();

    for (int i = 0; i < lmax; i++)
    {
        eval = toVal(i * 2);
        bta.add(0, eval);
    }

    dspElapsed("insert at pos 0      ", tstart1);

    //---------------
    bta.clear();

    tstart1 = std::chrono::system_clock::now();

    for (int i = 0; i < lmax; i++)
    {
        eval = toVal(i * 2);
        bta.add(i / 2, eval);
    }

    dspElapsed("insert in the middle ", tstart1);

    auto tstart2 = std::chrono::system_clock::now();

    for (int i = 0; i < lmax; i++)
    {
        int pos = i;
        eval = bta.get(pos);
    }
    eval = toVal(0);

    dspElapsed("get   ", tstart2);

    for (int t = 0; t <= 2; t++)
    {

        bta.clear();
        for (int i = 0; i < lmax; i++)
        {
            eval = toVal(i * 2);
            bta.add(i / 2, eval);

        }

        tstart2 = std::chrono::system_clock::now();

        auto tstart3 = std::chrono::system_clock::now();

        for (int i = 0; i < lmax; i++)
        {
            int pos;
            if (t == 0)
                pos = lmax - i - 1;
            if (t == 1)
                pos = 0;
            if (t == 2)
                pos = bta.size() / 2;
            bta.remove(pos);

        }
        char buf[256];
        sprintf(buf, "remove at %s", t == 0 ? "END" : t == 1 ? "0  " : "MID");
        dspElapsed(buf, tstart3);
        //dspElapsed("total*", tstart1);
    }

}

template<class ARR2>
void atestvalid(int lmax)
{

    printf("\nvalidation test comparing to std::vector.  insert,remove,get at random positions\n");

    std::vector<BTATYPE> a1;
    ARR2 a2;

    auto tstart = std::chrono::system_clock::now();

    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    std::srand(millis);

    for (int i = 0; i < lmax; i++)
    {
        BTATYPE val = toVal(std::rand());
        int pos = std::rand() % (a1.size() + 1);

        //a1.add(pos, val);
        a1.insert(a1.begin() + pos, val);
        a2.add(pos, val);

        assert(a1[pos] == a2[pos] && "insert");
        assert(a1[pos] == a2.get(pos) && "insert");

    }

    for (int i = 0;; i++)
    {
        assert(a1.size() == a2.size());
        if (i % 100000 == 0)
        {
            printf("test: oper %'i , size %'lu ", i, a1.size());
            for (unsigned j = 0; j < a1.size(); j++)
            {
                assert(a1[j] == a2.get(j));
                assert(a1[j] == a2[j]);
            }
            printf(" ok \n");
        }

        BTATYPE val = toVal(std::rand());
        int pos = std::rand() % (a1.size());

        assert(a1.size() == a2.size());

        BTATYPE v1 = a1[pos];
        BTATYPE v2 = a2.get(pos);
        assert(v1 == v2 && 1);
        assert(a1[pos] == v1);
        assert(a2[pos] == v2);

        int remv = std::rand() % 100;
        remv = remv >= 49;

        if (remv)
        {
            assert(a1[pos] == a2[pos]);
            a2.remove(pos);
            a1.erase(a1.begin() + pos);
            assert(a1.size() == a2.size() && "after rem 0");
            if (pos < (int) a1.size())
            {
                assert(a1[pos] == a2[pos] && "after rem 1");
            }

            assert(a1.size() == a2.size() && "after rem 2");
            if (a1.size() == 0)
                break;
        } else
        {

            val = toVal(std::rand());
            pos = std::rand() % (a1.size() + 1);
            a2.add(pos, val);
            a1.insert(a1.begin() + pos, val);
        }

    }

    dspElapsed("total ", tstart);

}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "en_US");
    printf("\ndata type: %s\n", printtype);
    int lmax = 1000000;
    atestspeed<BTAType>(lmax);
    atestvalid<BTAType>(100000);
    return 0;
}

