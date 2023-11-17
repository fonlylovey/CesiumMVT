
#include "VectorTileReader.h"
#include "mapmost_mvt_decryption_for_unreal_engine_static.h"

VTR::VectorTileReader::~VectorTileReader()
{

}

std::string VTR::VectorTileReader::readFile(const std::string& filePath)
{
    if (filePath.empty() || (filePath.size() == 1 && filePath[0] == '-'))
    {
        return std::string{ std::istreambuf_iterator<char>(std::cin.rdbuf()),
                        std::istreambuf_iterator<char>() };
    }

    std::ifstream stream{ filePath, std::ios_base::in | std::ios_base::binary };
    if (!stream)
    {
        throw std::runtime_error{ std::string{"Can not open file '"} + filePath + "'" };
    }

    stream.exceptions(std::ifstream::failbit);

    std::string buffer{ std::istreambuf_iterator<char>(stream.rdbuf()),
                    std::istreambuf_iterator<char>() };
    stream.close();

    return buffer;
}

void VTR::VectorTileReader::writeFile(const std::string& buffer, const std::string& filePath)
{
    std::ofstream stream{ filePath, std::ios_base::out | std::ios_base::binary };
    if (!stream)
    {
        throw std::runtime_error{ std::string{"Can not open file '"} + filePath + "'" };
    }

    stream.exceptions(std::ifstream::failbit);

    stream.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    stream.close();
}

std::string VTR::VectorTileReader::mvtDecode(const std::string& strData)
{
    return DecryptAESECB(strData);
}

std::string VTR::VectorTileReader::readVectorTile(const std::string& data, DataType type /*= DataType::Byte*/)
{
    m_type = type;
    m_data = (m_type == DataType::File) ? readFile(data) : data;
    return m_data;
}
