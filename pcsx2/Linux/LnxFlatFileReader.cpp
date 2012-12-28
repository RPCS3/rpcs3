#include "PrecompiledHeader.h"
#include "AsyncFileReader.h"

FlatFileReader::FlatFileReader(void)
{
	m_blocksize = 2048;
	m_fd = 0;
	m_aio_context = 0;
}

FlatFileReader::~FlatFileReader(void)
{
	Close();
}

bool FlatFileReader::Open(const wxString& fileName)
{
	m_filename = fileName;

	int err = io_setup(64, &m_aio_context);
	if (err) return false;

    m_fd = wxOpen(fileName, O_RDONLY, 0);

	return (m_fd != 0);
}

int FlatFileReader::ReadSync(void* pBuffer, uint sector, uint count)
{
	BeginRead(pBuffer, sector, count);
	return FinishRead();
}

void FlatFileReader::BeginRead(void* pBuffer, uint sector, uint count)
{
	u64 offset;
	offset = sector * (u64)m_blocksize + m_dataoffset;

	u32 bytesToRead = count * m_blocksize;

	struct iocb iocb;
	struct iocb* iocbs = &iocb;

	io_prep_pread(&iocb, m_fd, pBuffer, bytesToRead, offset);
	io_submit(m_aio_context, 1, &iocbs);
}

int FlatFileReader::FinishRead(void)
{
	u32 bytes;

	int min_nr = 1;
	int max_nr = 1;
    struct io_event* events = new io_event[max_nr];

	int event = io_getevents(m_aio_context, min_nr, max_nr, events, NULL);
	if (event < 1) {
		return -1;
	}

	return 1;
}

void FlatFileReader::CancelRead(void)
{
	// Will be done when m_aio_context context is destroyed
	// Note: io_cancel exists but need the iocb structure as parameter
	// int io_cancel(aio_context_t ctx_id, struct iocb *iocb,
	//                struct io_event *result);
}

void FlatFileReader::Close(void)
{

	if (m_fd) close(m_fd);

	io_destroy(m_aio_context);

	m_fd = 0;
	m_aio_context = 0;
}

uint FlatFileReader::GetBlockCount(void) const
{
	return (int)(Path::GetFileSize(m_filename) / m_blocksize);
}
