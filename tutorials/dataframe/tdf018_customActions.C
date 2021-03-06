/// \file
/// \ingroup tutorial_tdataframe
/// \notebook
/// This tutorial shows how to implement a custom action.
/// As an example, we build a helper for filling THns.
///
/// \macro_code
///
/// \date April 2018
/// \author Enrico Guiraud, Danilo Piparo

// This is a custom action which respects a well defined interface. It supports parallelism,
// in the sense that it behaves correctly if implicit multi threading is enabled.
// We template it on:
// - The type of the internal THnT(s)
// - The dimension of the internal THnT(s)
// - The types of the columns which we will use
// Note the plural: in presence of a MT execution, internally more than a single THnT is created.
template <typename T, unsigned int NDIM, typename... ColumnTypes>
class THnHelper : public ROOT::Detail::TDF::TActionImpl<THnHelper<T, NDIM, ColumnTypes...>> {
public:
   /// This is the list of the types of the columns, a requirement for every helper.
   using ColumnTypes_t = ROOT::TypeTraits::TypeList<ColumnTypes...>;
   /// This is a handy, expressive shortcut.
   using THn_t = THnT<T>;
   /// This type is a requirement for every helper.
   using Result_t = THn_t;

private:
   std::vector<std::shared_ptr<THn_t>> fHistos; // one per data processing slot
   const ROOT::Detail::TDF::ColumnNames_t fColumnNames;

public:
   /// This constructor takes all the parameters necessary to build the THnTs. In addition, it requires the names of
   /// the columns which will be used.
   THnHelper(std::string_view name, std::string_view title, std::array<int, NDIM> nbins, std::array<double, NDIM> xmins,
             std::array<double, NDIM> xmax, ROOT::Detail::TDF::ColumnNames_t columnNames)
      : fColumnNames(columnNames)
   {
      const auto nSlots = ROOT::GetImplicitMTPoolSize();
      for (auto i : ROOT::TSeqU(nSlots)) {
         fHistos.emplace_back(std::make_shared<THn_t>(std::string(name).c_str(), std::string(title).c_str(),
                                                      NDIM, nbins.data(), xmins.data(), xmax.data()));
         (void)i;
      }
   }
   THnHelper(THnHelper &&) = default;
   THnHelper(const THnHelper &) = delete;
   ROOT::Detail::TDF::ColumnNames_t GetColumnNames() const { return fColumnNames; }
   std::shared_ptr<THn_t> GetResultPtr() const { return fHistos[0]; }
   void Initialize() {}
   void InitTask(TTreeReader *, unsigned int) {}
   /// This is a method executed at every entry. The types of the parameters are the ones with which we
   /// templated the Helper.
   void Exec(unsigned int slot, ColumnTypes... values)
   {
      // Since THnT<T>::Fill expects a double*, we build it passing through a std::array.
      std::array<double, sizeof...(ColumnTypes)> valuesArr{static_cast<double>(values)...};
      fHistos[slot]->Fill(valuesArr.data());
   }
   /// This method is called at the end of the event loop. It is used to merge all the internal THnTs which
   /// were used in each of the data processing slots.
   void Finalize()
   {
      auto &res = fHistos[0];
      for (auto slot : ROOT::TSeqU(1, fHistos.size())) {
         res->Add(fHistos[slot].get());
      }
   }
};

void tdf018_customActions()
{
   // We enable implicit parallelism
   ROOT::EnableImplicitMT();

   // We create an empty TDataFrame which contains 4 columns filled with random numbers.
   // The type of the numbers held by the columns are: double, double, float, int.
   ROOT::Experimental::TDataFrame d(128);
   auto genD = []() { return gRandom->Uniform(-5, 5); };
   auto genF = [&genD]() { return (float)genD(); };
   auto genI = [&genD]() { return (int)genD(); };
   auto dd = d.Define("x0", genD).Define("x1", genD).Define("x2", genF).Define("x3", genI);

   // Our Helper type: templated on the internal THnT type, the size, the types of the columns
   // we'll use to fill.
   using Helper_t = THnHelper<float, 4, double, double, float, int>;

   Helper_t helper{"myThN",                          // Name
                   "A THn with 4 dimensions",        // Title
                   {4, 4, 8, 2},                     // NBins
                   {-10., -10, -4., -6.},            // Axes min values
                   {10., 10, 5., 7.},                // Axes max values
                   {"x0", "x1", "x2", "x3", "x4"}};

   // We book the action: it will be treated during the event loop.
   auto myTHnT = dd.Book(std::move(helper));

   myTHnT->Print();
}
