/*
    Code taken from project https://github.com/ClaudiuHBann/Web_Scraper_v2

    Need some functionality? Hit me up or Fork it :P
*/

#include <Mshtml.h>
#include <atlstr.h>
#include <io.h>
#include <fcntl.h>

#include <iostream>
#include <vector>
#include <string>

using namespace std;

thread_local size_t HTMLParserInstanceCount = 0;

class HTMLParser
{
  public:
    HTMLParser()
    {
        if (!HTMLParserInstanceCount++)
        {
            auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (result != S_OK && result != S_FALSE && result != RPC_E_CHANGED_MODE)
            {
                HTMLParserInstanceCount--;
            }
        }
    }

    ~HTMLParser()
    {
        Uninitialize7();
        Uninitialize3();
        Uninitialize2();

        if (!--HTMLParserInstanceCount)
        {
            CoUninitialize();
        }
    }

    IHTMLDocument2 *GetIHTMLDocument2()
    {
        Initialize2();

        return mIHTMLDocument2;
    }

    IHTMLDocument3 *GetIHTMLDocument3()
    {
        Initialize3();

        return mIHTMLDocument3;
    }

    IHTMLDocument7 *GetIHTMLDocument7()
    {
        Initialize7();

        return mIHTMLDocument7;
    }

    bool Parse(const wstring &html)
    {
        if (!Initialize2())
        {
            return false;
        }

        CString htmlCString(html.c_str());

        auto safeArray = SafeArrayCreateVector(VT_VARIANT, 0, 1);
        if (!safeArray)
        {
            return false;
        }

        VARIANT *variant{};
        auto result = SafeArrayAccessData(safeArray, (void **)&variant);
        if (!variant)
        {
            SafeArrayDestroy(safeArray);
            return false;
        }

        variant->vt = VT_BSTR;
        variant->bstrVal = htmlCString.AllocSysString();

        result = mIHTMLDocument2->write(safeArray);
        mIHTMLDocument2->close();

        SysFreeString(variant->bstrVal);
        SafeArrayDestroy(safeArray);

        return SUCCEEDED(result);
    }

    static inline IHTMLElementCollection *GetElementAllAsCollection(IHTMLElement *element)
    {
        if (!element)
        {
            return nullptr;
        }

        IDispatch *childrenRaw{};
        auto result = element->get_all(&childrenRaw);
        if (FAILED(result))
        {
            return nullptr;
        }

        IHTMLElementCollection *children{};
        result = childrenRaw->QueryInterface(IID_IHTMLElementCollection, reinterpret_cast<void **>(&children));
        return children;
    }

    static inline IHTMLElement *GetElementFromCollectionByIndex(IHTMLElementCollection *collection, const long index)
    {
        if (!collection)
        {
            return nullptr;
        }

        VARIANT variantName{};
        variantName.vt = VT_UINT;
        variantName.lVal = index;

        VARIANT variantIndex{};
        variantIndex.vt = VT_I4;
        variantIndex.intVal = 0;

        IDispatch *iDispatch{};
        auto result = collection->item(variantName, variantIndex, &iDispatch);
        if (!iDispatch)
        {
            return nullptr;
        }

        IHTMLElement *element{};
        result = iDispatch->QueryInterface(IID_IHTMLElement, (void **)&element);
        return element;
    }

    static inline bool ElementHasAttribute(IHTMLElement *element, const pair<wstring, wstring> &attribute)
    {
        return GetElementAttributeValueByName(element, attribute.first) == attribute.second;
    }

    static bool ElementHasAttributes(IHTMLElement *element, const vector<pair<wstring, wstring>> &attributes)
    {
        if (!element)
        {
            return false;
        }

        for (const auto &attribute : attributes)
        {
            if (!ElementHasAttribute(element, attribute))
            {
                return false;
            }
        }

        return true;
    }

    inline wstring GetScriptAsString(const long index)
    {
        IHTMLElementCollection *scripts{};
        GetIHTMLDocument2()->get_scripts(&scripts);
        if (!scripts)
        {
            return {};
        }

        auto script = GetElementFromCollectionByIndex(scripts, index);
        if (!script)
        {
            return {};
        }

        return GetElementInnerHTML(script);
    }

    static inline wstring GetElementInnerHTML(IHTMLElement *element)
    {
        if (!element)
        {
            return {};
        }

        BSTR bstr;
        element->get_innerHTML(&bstr);
        return bstr ? bstr : wstring();
    }

    static inline wstring GetElementInnerText(IHTMLElement *element)
    {
        if (!element)
        {
            return {};
        }

        BSTR bstr;
        element->get_innerText(&bstr);
        return bstr ? bstr : wstring();
    }

    inline IHTMLElement *GetElementById(const wstring &id)
    {
        BSTR bStr = SysAllocString(id.c_str());
        IHTMLElement *element{};
        GetIHTMLDocument3()->getElementById(bStr, &element);
        SysFreeString(bStr);
        return element;
    }

    inline vector<IHTMLElement *> GetElementsByAttributes(const vector<pair<wstring, wstring>> &attributes)
    {
        IHTMLElementCollection *collection{};
        auto result = GetIHTMLDocument2()->get_all(&collection);
        if (FAILED(result))
        {
            return {};
        }

        return GetCollectionElementsByAttributes(collection, attributes);
    }

    static vector<IHTMLElement *> GetCollectionElementsByAttributes(IHTMLElementCollection *collection,
                                                                    const vector<pair<wstring, wstring>> &attributes)
    {
        if (!collection)
        {
            return {};
        }

        vector<IHTMLElement *> htmlElements{};

        long collectionLength;
        collection->get_length(&collectionLength);
        for (long i = 0; i < collectionLength; i++)
        {
            auto element = GetElementFromCollectionByIndex(collection, i);
            if (element && ElementHasAttributes(element, attributes))
            {
                htmlElements.push_back(element);
            }
        }

        return htmlElements;
    }

    inline IHTMLElementCollection *GetElementsByClassName(const wstring &name)
    {
        BSTR bStr = SysAllocString(name.c_str());
        IHTMLElementCollection *collection{};
        GetIHTMLDocument7()->getElementsByClassName(bStr, &collection);
        SysFreeString(bStr);
        return collection;
    }

    static inline wstring GetElementAttributeValueByName(IHTMLElement *element, const wstring &name)
    {
        if (!element)
        {
            return {};
        }

        VARIANT variantAttribute{};
        variantAttribute.vt = VT_BSTR;

        CString attributeNameCString(name.c_str());
        auto attributeNameBStr = attributeNameCString.AllocSysString();

        auto result = element->getAttribute(attributeNameBStr, 0, &variantAttribute);
        SysFreeString(attributeNameBStr);

        if (SUCCEEDED(result) && variantAttribute.vt == VT_BSTR && variantAttribute.bstrVal)
        {
            return variantAttribute.bstrVal;
        }
        else
        {
            return {};
        }
    }

  private:
    IHTMLDocument2 *mIHTMLDocument2{};
    IHTMLDocument3 *mIHTMLDocument3{};
    IHTMLDocument7 *mIHTMLDocument7{};

    inline bool Initialize2()
    {
        if (!HTMLParserInstanceCount)
        {
            return false;
        }

        if (mIHTMLDocument2)
        {
            return true;
        }

        return SUCCEEDED(CoCreateInstance(CLSID_HTMLDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IHTMLDocument2,
                                          (void **)&mIHTMLDocument2));
    }

    inline bool Uninitialize2()
    {
        if (!mIHTMLDocument2)
        {
            return true;
        }

        return !mIHTMLDocument2->Release();
    }

    inline bool Initialize3()
    {
        if (!Initialize2())
        {
            return false;
        }

        if (mIHTMLDocument3)
        {
            return true;
        }

        return SUCCEEDED(mIHTMLDocument2->QueryInterface(IID_IHTMLDocument3, (void **)&mIHTMLDocument3));
    }

    inline bool Uninitialize3()
    {
        if (!mIHTMLDocument3)
        {
            return true;
        }

        return !mIHTMLDocument3->Release();
    }

    inline bool Initialize7()
    {
        if (!Initialize2())
        {
            return false;
        }

        if (mIHTMLDocument7)
        {
            return true;
        }

        return SUCCEEDED(mIHTMLDocument2->QueryInterface(IID_IHTMLDocument7, (void **)&mIHTMLDocument7));
    }

    inline bool Uninitialize7()
    {
        if (!mIHTMLDocument7)
        {
            return true;
        }

        return !mIHTMLDocument7->Release();
    }
};

#include <winrt/windows.foundation.h>
#include <winrt/windows.web.http.h>

using namespace winrt;
using namespace Windows;
using namespace Foundation;
using namespace Web::Http;

int main()
{
    auto &&_ = _setmode(_fileno(stdout), _O_U16TEXT);
    _;

    Uri uri(L"https://github.com/ClaudiuHBann/Web_Scraper_v2");

    HttpClient client;
    auto &&html = client.GetStringAsync(uri).get();

    HTMLParser parser;
    parser.Parse(html.data());

    auto elementByID = parser.GetElementById(L"fork-button");

    wcout << format(L"Element with ID: {} has HTML: {}", L"fork-button", HTMLParser::GetElementInnerHTML(elementByID))
          << endl;
    wcout << format(L"Element with ID: {} has Text: {}", L"fork-button", HTMLParser::GetElementInnerText(elementByID))
          << endl
          << endl;

    auto elements = parser.GetElementsByAttributes({{L"type", L"button"}});
    for (auto element : elements)
    {
        auto &&elementText = HTMLParser::GetElementInnerText(element);
        if (!elementText.empty())
        {
            wcout << format(LR"(Button with attribute {}="{}" has text: {})", L"type", L"button", elementText) << endl;
        }

        auto elementChildren = HTMLParser::GetElementAllAsCollection(element);
        long elementChildrenCount;
        elementChildren->get_length(&elementChildrenCount);
        wcout << format(LR"(Button with attribute {}="{}" has {} children)", L"type", L"button", elementChildrenCount)
              << endl;
    }
    wcout << endl;

    wcout << format(L"Script 1:\n{}", parser.GetScriptAsString(0)) << endl;

    return 0;
}
