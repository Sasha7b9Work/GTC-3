// 2023/09/08 22:14:01 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Storage/MemoryStorage.h"
#include "Utils/Math.h"
#include "Modules/W25Q80DV/W25Q80DV.h"
#include "Hardware/Timer.h"
#include "Storage/Storage.h"
#include <cstring>
#include <cstdlib>


namespace MemoryStorage
{
    static const uint BEGIN = 0x8000000 + 200 * 1024;
    static const uint END = 0x8000000 + 250 * 1024;

#pragma pack(1)
    
    struct Record
    {
        int           number;
        Measurements  measurements;
        uint          crc;
        uint          control_field;     // ��� ����� ��� �������� ������������ ������

        Measurements *GetMeasurements()
        {
            return &measurements;
        }

        void *Begin()
        {
            return (void *)this;
        }

        void *End()
        {
            return (void *)(this + 1);
        }

        void Write(int _number, const Measurements &meas)
        {
            Record data;

            data.number = _number;
            std::memcpy(&data.measurements, &meas, sizeof(Measurements));
            data.crc = Math::CalculateCRC(&data, sizeof(number) + sizeof(Measurements));
            data.control_field = 0;

            W25Q80DV::WriteData((uint)Begin(), (const void *)&data, sizeof(data));
        }

        bool IsEmpty()                          // ���� ����� ���� ����������� ������
        {
            for (uint *address = (uint *)this; address < (uint *)End(); address++) //-V1032
            {
                if (*address != (uint)(-1))
                {
                    return false;
                }
            }

            return true;
        }

        bool IsErased()                         // ������ �����
        {
            return number == 0;
        }

        bool IsValid()
        {
            if ((uint)number == (uint)-1 || number == 0 || control_field != 0)
            {
                return false;
            }

            return Math::CalculateCRC(&number, sizeof(measurements) + sizeof(number)) == crc;
        }

        void Erase()
        {
            W25Q80DV::WriteUInt((uint)Begin(), 0);
        }

        static Record *Oldest();

        static Record *Newest();

        static Record *ForMeasurements(const Measurements *);
    };

    struct Page
    {
        void Init(int num_page)
        {
            startAddress = BEGIN + W25Q80DV::SIZE_PAGE * (uint)num_page;
        }

        Record *FirstRecord()
        {
            return (Record *)startAddress;
        }

        int GetMaxRecordsCount() const
        {
            return W25Q80DV::SIZE_PAGE / sizeof(Record);
        }

        int GetRecordsCount()
        {
            int result = 0;

            for (Record *record = FirstRecord(); record < LastRecord(); record++)
            {
                if (record->IsValid())
                {
                    result++;
                }
            }

            return result;
        }

        Record *LastRecord()
        {
            return FirstRecord() + GetMaxRecordsCount();

        }

        void Prepare()
        {
            for (Record *record = FirstRecord(); record < LastRecord(); record++)
            {
                if (record->IsEmpty() || record->IsErased())
                {
                    continue;
                }

                if (!record->IsValid())
                {
                    record->Erase();
                }
            }
        }

        // ���������� ������ ��������� ��� ������ �������� ��� nullptr, ���� ��� ������
        static Page *GetFirstForRecord();

        // ���������� �������� � ����� ������ �������
        static Page *GetWithOldestRecord();

        Record *GetFirstEmptyRecord()
        {
            for (Record *record = FirstRecord(); record < LastRecord(); record++)
            {
                if (record->IsEmpty())
                {
                    return record;
                }
            }

            return nullptr;
        }

        Record *Append(const Measurements &measurements);

        void Erase()
        {
            int num_page = (int)(startAddress / W25Q80DV::SIZE_PAGE);

            W25Q80DV::ErasePage(num_page);
        }

        void *Begin()
        {
            return (void *)startAddress;
        }

        bool IsEmpty()
        {
            for (Record *record = FirstRecord(); record < LastRecord(); record++)
            {
                if (record->IsEmpty())
                {
                    continue;
                }

                return false;
            }

            return true;
        }

        int GetLastNumber()
        {
            int result = 0;

            for (Record *record = FirstRecord(); record < LastRecord(); record++)
            {
                if (record->IsValid() && record->number > result)
                {
                    result = record->number;
                }
            }

            return result;
        }

        Record *GetOldestRecord()
        {
            Record *result = nullptr;

            for (Record *record = FirstRecord(); record < LastRecord(); record++)
            {
                if (record->IsValid())
                {
                    if (!result || (record->number < result->number))
                    {
                        result = record;
                    }
                }
            }

            return result;
        }

        Record *GetNewestRecord()
        {
            Record *result = nullptr;

            for (Record *record = FirstRecord(); record < LastRecord(); record++)
            {
                if (record->IsValid())
                {
                    if (!result || (record->number > result->number))
                    {
                        result = record;
                    }
                }
            }

            return result;
        }

    private:

        uint startAddress;
    };

    static const int NUM_PAGES = (END - BEGIN) / W25Q80DV::SIZE_PAGE;

    static Page pages[NUM_PAGES];

    static int GetNewNumber()
    {
        int result = 0;

        for (int i = 0; i < NUM_PAGES; i++)
        {
            Page &page = pages[i];

            if (page.IsEmpty())
            {
                continue;
            }

            if (page.GetLastNumber() > result)
            {
                result = page.GetLastNumber();
            }
        }

        return result + 1;
    }

    Page *Page::GetWithOldestRecord()
    {
        Page *result = nullptr;

        for (int i = 0; i < NUM_PAGES; i++)
        {
            Page &page = pages[i];

            if (page.IsEmpty())
            {
                continue;
            }

            int last_number = page.GetLastNumber();

            if (last_number != 0)
            {
                if (!result || (last_number < result->GetLastNumber()))
                {
                    result = &page;
                }
            }
        }

        return result;
    }

    Page *Page::GetFirstForRecord()
    {
        for (int i = 0; i < NUM_PAGES; i++)
        {
            if (pages[i].GetFirstEmptyRecord())
            {
                return &pages[i];
            }
        }

        return nullptr;
    }

    Record *Page::Append(const Measurements &measurements)
    {
        Record *record = GetFirstEmptyRecord();

        if (!record)
        {
            Erase();

            record = (Record *)Begin();
        }

        record->Write(GetNewNumber(), measurements);

        return record;
    }

    // ��������� ��� ������� �� ������� ����������� ������� � ������� ��
    static void Prepare();

    int NumberOldestRecord();
    int NumberNewestRecord();

    static int GetRecordsCount();
}


void MemoryStorage::Init()
{
    for (int i = 0; i < NUM_PAGES; i++)
    {
        pages[i].Init(i);
    }

    Prepare();
}


void MemoryStorage::Erase(const Measurements *meas)
{
    Record::ForMeasurements(meas)->Erase();
}


void *MemoryStorage::Append(const Measurements &meas)
{
    Page *page = Page::GetFirstForRecord();

    if (!page)
    {
        page = Page::GetWithOldestRecord();
    }

    return page->Append(meas);
}


const Measurements *MemoryStorage::GetOldest()
{
    Record *record = Record::Oldest();

    return record ? record->GetMeasurements() : nullptr;
}


void MemoryStorage::Prepare()
{
    for (int i = 0; i < NUM_PAGES; i++)
    {
        pages[i].Prepare();
    }
}


MemoryStorage::Record *MemoryStorage::Record::Oldest()
{
    Record *result = nullptr;

    for (int i = 0; i < NUM_PAGES; i++)
    {
        Record *oldest = pages[i].GetOldestRecord();

        if (oldest)
        {
            if (!result || oldest->number < result->number)
            {
                result = oldest;
            }
        }
    }

    return result;
}


MemoryStorage::Record *MemoryStorage::Record::Newest()
{
    Record *result = nullptr;

    for (int i = 0; i < NUM_PAGES; i++)
    {
        Record *newest = pages[i].GetNewestRecord();

        if (newest)
        {
            if (!result || newest->number > result->number)
            {
                result = newest;
            }
        }
    }

    return result;
}


int MemoryStorage::NumberOldestRecord()
{
    Record *record = Record::Oldest();

    return record ? record->number : 0;
}


int MemoryStorage::NumberNewestRecord()
{
    Record *record = Record::Newest();

    return record ? record->number : 0;
}


int MemoryStorage::GetRecordsCount()
{
    int result = 0;

    for (int i = 0; i < NUM_PAGES; i++)
    {
        result += pages[i].GetRecordsCount();
    }

    return result;
}


MemoryStorage::Record *MemoryStorage::Record::ForMeasurements(const Measurements *meas)
{
    uint *pointer = (uint *)meas;

    pointer--;

    return (MemoryStorage::Record *)pointer;
}


namespace MemoryStorage
{
    namespace NSTest
    {
        static void AppendMeasure()
        {
//            Measurements meas;

//            meas.SetFullMeasure(Measurer::Measure1Min());
        }

        static void Fill()
        {
            const int num_records = 1150 * 10;

            for (int i = 0; i < num_records; i++)
            {
                AppendMeasure();
            }
        }

        // ��������� ���������
        static void Corrupt()
        {
            for (int i = 0; i < 100; i++)
            {
                uint value = 0;
                uint address = 0;

                while (value == 0)
                {
                    address = (uint)(BEGIN + ((uint)std::rand() % (END - BEGIN)));

                    address &= 0xFFFFFFFFFC;

                    value = *((uint *)address);
                }

                value = 0;

                W25Q80DV::WriteUInt(address, value);

                Prepare();
            }
        }

        static void ReadAll()
        {
            while (GetRecordsCount())
            {
                const Measurements *meas = MemoryStorage::GetOldest();

//                Record *record = Record::ForMeasurements(meas);
//
//                LOG_WRITE("erase record %X number %d for measure %X, all %d, first %d, last %d",
//                    record, record->number, meas, GetRecordsCount(), NumberOldestRecord(), NumberNewestRecord());

                MemoryStorage::Erase(meas);
            }
        }
    }
}


void MemoryStorage::Test()
{
    NSTest::Fill();

    NSTest::Corrupt();

    NSTest::ReadAll();
}


#include "MemoryStorage_test.inc"
