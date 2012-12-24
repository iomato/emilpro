#include "../test.hh"

#include <symbolfactory.hh>
#include <isymbolprovider.hh>
#include <idisassembly.hh>

#include <utils.hh>

#include <unordered_map>

using namespace emilpro;

class FactoryFixture : public ISymbolListener, public IDisassembly::IInstructionListener
{
public:
	FactoryFixture()
	{
		SymbolFactory &factory = SymbolFactory::instance();

		factory.registerListener(this);
	}

	~FactoryFixture()
	{
		SymbolFactory &factory = SymbolFactory::instance();

		factory.destroy();
	}

	void onSymbol(ISymbol &sym)
	{
		m_symbolNames[sym.getName()] = &sym;
		m_symbolAddrs[sym.getAddress()] = &sym;
	}

	void onInstruction(off_t offset, const char *ascii)
	{
		printf("%03x: %s\n", offset, ascii);
		m_instructions[offset] = std::string(ascii);
	}

	std::unordered_map<std::string, ISymbol *> m_symbolNames;
	std::unordered_map<uint64_t, ISymbol *> m_symbolAddrs;

	std::unordered_map<uint64_t, std::string> m_instructions;
};

TESTSUITE(symbol_provider)
{
	TEST(nonPerfectMatches, FactoryFixture)
	{
		SymbolFactory &factory = SymbolFactory::instance();
		unsigned res;

		res = factory.parseBestProvider(NULL, 0);
		ASSERT_TRUE(res != ISymbolProvider::PERFECT_MATCH);

		char notElf[] = "\177ElF-ngt-annat";
		res = factory.parseBestProvider(&notElf, sizeof(notElf));
		ASSERT_TRUE(res != ISymbolProvider::PERFECT_MATCH);

		char unparseableElf[] = "\177ELFjunk";
		res = factory.parseBestProvider(&unparseableElf, sizeof(unparseableElf));
		ASSERT_TRUE(res != ISymbolProvider::PERFECT_MATCH);
	};

	TEST(validElf, FactoryFixture)
	{
		SymbolFactory &factory = SymbolFactory::instance();
		unsigned res;

		size_t sz;
		void *data = read_file(&sz, "%s/test-binary", crpcut::get_start_dir());
		ASSERT_TRUE(data != (void *)NULL);

		res = factory.parseBestProvider(data, sz);
		ASSERT_TRUE(res > ISymbolProvider::NO_MATCH);

		ASSERT_TRUE(m_symbolNames.find("main") != m_symbolNames.end());
		ASSERT_TRUE(m_symbolNames.find("global_data") != m_symbolNames.end());

		ISymbol *sym = m_symbolNames["main"];
		ASSERT_TRUE(sym != (void *)NULL);

		ASSERT_TRUE(sym->getSize() > 1);
		ASSERT_TRUE(sym->getDataPtr() != (void *)NULL);
		IDisassembly &dis = IDisassembly::getInstance();

		ASSERT_TRUE(m_instructions.size() == 0U);
		// Disassemble main()
		dis.execute(this, sym->getDataPtr(), sym->getSize());

		ASSERT_TRUE(m_instructions.size() > 0U);
	}
}