
static const double threshold[] = {
  0.37824898958206177, 0.37828025221824646, 0.57707870006561279, 
  0.5943458080291748, 0.72520214319229126, 0.72614741325378418, 
  0.81433647871017456, 0.83435320854187012, 0.83671116828918457, 
  0.84519827365875244, 0.89991474151611328, 0.94097602367401123, 
  1.0162490606307983, 1.0177488327026367, 1.1509618759155273, 
  1.2161508798599243, 1.4199135303497314, 1.4216587543487549, 
  1.8539369106292725, 0.37942397594451904, 0.57710814476013184, 
  0.57716536521911621, 0.62982547283172607, 0.72511821985244751, 
  0.72525143623352051, 0.72610414028167725, 0.79467117786407471, 
  0.82472026348114014, 0.86904549598693848, 0.93287348747253418, 
  0.96159464120864868, 1.006117582321167, 1.0139410495758057, 
  1.0625166893005371, 1.1800283193588257, 1.5667153596878052, 
  1.7991418838500977, 7.6784458160400391, 71.5, 72.5, 73.5, 77.5, 78.5, 82.5, 
  85.5, 102.5, 103.5, 108.5, 109.5, 110.5, 113.5, 115.5, 117.5, 122.5, 173.5, 
  176.5, 177.5, 180.5, 181.5, 182.5, 183.5, 186.5, 188.5, 98.5, 104.5, 106.5, 
  113.5, 117.5, 121.5, 122.5, 123.5, 124.5, 127.5, 132.5, 134.5, 136.5, 138.5, 
  139.5, 143.5, 145.5, 146.5, 147.5, 148.5, 153.5, 154.5, 158.5, 161.5, 65.5, 
  90.5, 91.5, 96.5, 99.5, 101.5, 102.5, 103.5, 104.5, 105.5, 109.5, 110.5, 
  118.5, 119.5, 120.5, 121.5, 125.5, 181.5, 
};
static const int th_begin[] = {
  0, 19, 38, 63, 87, 
};
static const int th_len[] = {
  19, 19, 25, 24, 18, 
};

#include <stdlib.h>

/*
 * \brief function to convert a feature value into bin index.
 * \param val feature value, in floating-point
 * \param fid feature identifier
 * \return bin index corresponding to given feature value
 */
static inline int quantize(float val, unsigned fid) {
  const size_t offset = th_begin[fid];
  const double* array = &threshold[offset];
  int len = th_len[fid];
  int low = 0;
  int high = len;
  int mid;
  double mval;
  // It is possible th_begin[i] == [total_num_threshold]. This means that
  // all features i, (i+1), ... are not used for any of the splits in the model.
  // So in this case, just return something
  if (offset == 105 || val < array[0]) {
    return -10;
  }
  while (low + 1 < high) {
    mid = (low + high) / 2;
    mval = array[mid];
    if (val == mval) {
      return mid * 2;
    } else if (val < mval) {
      high = mid;
    } else {
      low = mid;
    }
  }
  if (array[low] == val) {
    return low * 2;
  } else if (high == len) {
    return len * 2;
  } else {
    return low * 2 + 1;
  }
}

#include "ls_sleep_predict.h"

const unsigned char is_categorical[] = {
  0, 0, 0, 0, 0, 
};

size_t get_num_output_group(void) {
  return 1;
}

size_t get_num_feature(void) {
  return 5;
}

static inline float pred_transform(float margin) {
  const float alpha = (float)1;
  return 1.0f / (1 + expf(-alpha * margin));
}

float predict_sleep_status(union Entry* data, int pred_margin) {

  for (int i = 0; i < 5; ++i) {
    if (data[i].missing != -1 && !is_categorical[i]) {
      data[i].qvalue = quantize(data[i].fvalue, i);
    }
  }
  float sum = 0.0f;
  // unsigned int tmp;
  // int nid, cond, fid;  /* used for folded subtrees */
  if (!(data[0].missing != -1) || (data[0].qvalue < 20)) {
    if (!(data[2].missing != -1) || (data[2].qvalue < 2)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 24)) {
        if (!(data[1].missing != -1) || (data[1].qvalue < 28)) {
          sum += (float)0.21405406296253204;
        } else {
          sum += (float)-0.09855072945356369;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 4)) {
          sum += (float)0.15272727608680725;
        } else {
          sum += (float)-0.39000749588012695;
        }
      }
    } else {
      if (!(data[2].missing != -1) || (data[2].qvalue < 44)) {
        if (!(data[1].missing != -1) || (data[1].qvalue < 22)) {
          sum += (float)0.3499428927898407;
        } else {
          sum += (float)0.18364867568016052;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 22)) {
          sum += (float)0.27368423342704773;
        } else {
          sum += (float)-0.36866256594657898;
        }
      }
    }
  } else {
    if (!(data[1].missing != -1) || (data[1].qvalue < 14)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 14)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 22)) {
          sum += (float)0.32016953825950623;
        } else {
          sum += (float)0.19417808949947357;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 14)) {
          sum += (float)0.23695345222949982;
        } else {
          sum += (float)-0.060407441109418869;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 20)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 10)) {
          sum += (float)-0.33439156413078308;
        } else {
          sum += (float)-0.17852135002613068;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 12)) {
          sum += (float)-0.33008068799972534;
        } else {
          sum += (float)-0.38449695706367493;
        }
      }
    }
  }
  if (!(data[1].missing != -1) || (data[1].qvalue < 24)) {
    if (!(data[2].missing != -1) || (data[2].qvalue < 2)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 28)) {
        if (!(data[0].missing != -1) || (data[0].qvalue < 34)) {
          sum += (float)0.14027795195579529;
        } else {
          sum += (float)-0.17283435165882111;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 4)) {
          sum += (float)-0.0021007454488426447;
        } else {
          sum += (float)-0.33031785488128662;
        }
      }
    } else {
      if (!(data[2].missing != -1) || (data[2].qvalue < 44)) {
        if (!(data[0].missing != -1) || (data[0].qvalue < 22)) {
          sum += (float)0.28883063793182373;
        } else {
          sum += (float)0.13892912864685059;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 20)) {
          sum += (float)0.23907391726970673;
        } else {
          sum += (float)-0.31244528293609619;
        }
      }
    }
  } else {
    if (!(data[0].missing != -1) || (data[0].qvalue < 8)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 10)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 24)) {
          sum += (float)0.29342770576477051;
        } else {
          sum += (float)0.16199386119842529;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 12)) {
          sum += (float)0.21216414868831635;
        } else {
          sum += (float)-0.057743381708860397;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 24)) {
        if (!(data[0].missing != -1) || (data[0].qvalue < 32)) {
          sum += (float)-0.0078379642218351364;
        } else {
          sum += (float)-0.19710145890712738;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 16)) {
          sum += (float)-0.27968987822532654;
        } else {
          sum += (float)-0.32259500026702881;
        }
      }
    }
  }
  if (!(data[1].missing != -1) || (data[1].qvalue < 18)) {
    if (!(data[2].missing != -1) || (data[2].qvalue < 4)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 30)) {
        if (!(data[3].missing != -1) || (data[3].qvalue < 6)) {
          sum += (float)-0.1602964848279953;
        } else {
          sum += (float)0.051497597247362137;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 4)) {
          sum += (float)0.01238115131855011;
        } else {
          sum += (float)-0.29163253307342529;
        }
      }
    } else {
      if (!(data[2].missing != -1) || (data[2].qvalue < 42)) {
        if (!(data[0].missing != -1) || (data[0].qvalue < 12)) {
          sum += (float)0.2543967068195343;
        } else {
          sum += (float)0.13607056438922882;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 28)) {
          sum += (float)0.12971605360507965;
        } else {
          sum += (float)-0.28381815552711487;
        }
      }
    }
  } else {
    if (!(data[0].missing != -1) || (data[0].qvalue < 8)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 18)) {
        if (!(data[3].missing != -1) || (data[3].qvalue < 46)) {
          sum += (float)0.22331085801124573;
        } else {
          sum += (float)0.046362239867448807;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 12)) {
          sum += (float)0.17549590766429901;
        } else {
          sum += (float)-0.055900819599628448;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 26)) {
        if (!(data[4].missing != -1) || (data[4].qvalue < 4)) {
          sum += (float)-0.083708606660366058;
        } else {
          sum += (float)-0.19328711926937103;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 16)) {
          sum += (float)-0.24181579053401947;
        } else {
          sum += (float)-0.28334316611289978;
        }
      }
    }
  }
  if (!(data[0].missing != -1) || (data[0].qvalue < 24)) {
    if (!(data[4].missing != -1) || (data[4].qvalue < 28)) {
      if (!(data[1].missing != -1) || (data[1].qvalue < 26)) {
        if (!(data[4].missing != -1) || (data[4].qvalue < 0)) {
          sum += (float)0.10585574060678482;
        } else {
          sum += (float)0.24591004848480225;
        }
      } else {
        if (!(data[0].missing != -1) || (data[0].qvalue < 4)) {
          sum += (float)0.17247278988361359;
        } else {
          sum += (float)-0.0029273766558617353;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 10)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 8)) {
          sum += (float)-0.064832970499992371;
        } else {
          sum += (float)0.2065703421831131;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 32)) {
          sum += (float)-0.18183931708335876;
        } else {
          sum += (float)0.043365471065044403;
        }
      }
    }
  } else {
    if (!(data[1].missing != -1) || (data[1].qvalue < 6)) {
      if (!(data[3].missing != -1) || (data[3].qvalue < 44)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 2)) {
          sum += (float)-0.20606295764446259;
        } else {
          sum += (float)0.18608520925045013;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 26)) {
          sum += (float)0.070914529263973236;
        } else {
          sum += (float)-0.092336595058441162;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 18)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 10)) {
          sum += (float)-0.23059502243995667;
        } else {
          sum += (float)-0.066831551492214203;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 10)) {
          sum += (float)-0.19903218746185303;
        } else {
          sum += (float)-0.25687944889068604;
        }
      }
    }
  }
  if (!(data[1].missing != -1) || (data[1].qvalue < 20)) {
    if (!(data[4].missing != -1) || (data[4].qvalue < 28)) {
      if (!(data[0].missing != -1) || (data[0].qvalue < 18)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 32)) {
          sum += (float)0.22547805309295654;
        } else {
          sum += (float)0.075272731482982635;
        }
      } else {
        if (!(data[1].missing != -1) || (data[1].qvalue < 2)) {
          sum += (float)0.15942706167697906;
        } else {
          sum += (float)0.0083231236785650253;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 8)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 6)) {
          sum += (float)-0.047651931643486023;
        } else {
          sum += (float)0.19322051107883453;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 36)) {
          sum += (float)-0.1493418961763382;
        } else {
          sum += (float)0.03992127999663353;
        }
      }
    }
  } else {
    if (!(data[0].missing != -1) || (data[0].qvalue < 16)) {
      if (!(data[3].missing != -1) || (data[3].qvalue < 38)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 4)) {
          sum += (float)-0.20750145614147186;
        } else {
          sum += (float)0.1583055704832077;
        }
      } else {
        if (!(data[0].missing != -1) || (data[0].qvalue < 0)) {
          sum += (float)0.081509575247764587;
        } else {
          sum += (float)-0.10257355123758316;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 28)) {
        if (!(data[1].missing != -1) || (data[1].qvalue < 36)) {
          sum += (float)-0.056478321552276611;
        } else {
          sum += (float)-0.15182261168956757;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 20)) {
          sum += (float)-0.18487350642681122;
        } else {
          sum += (float)-0.23609264194965363;
        }
      }
    }
  }
  if (!(data[0].missing != -1) || (data[0].qvalue < 26)) {
    if (!(data[2].missing != -1) || (data[2].qvalue < 4)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 24)) {
        if (!(data[3].missing != -1) || (data[3].qvalue < 6)) {
          sum += (float)-0.29736799001693726;
        } else {
          sum += (float)0.045562904328107834;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 4)) {
          sum += (float)0.11008671671152115;
        } else {
          sum += (float)-0.24595955014228821;
        }
      }
    } else {
      if (!(data[2].missing != -1) || (data[2].qvalue < 34)) {
        if (!(data[1].missing != -1) || (data[1].qvalue < 12)) {
          sum += (float)0.19384889304637909;
        } else {
          sum += (float)0.078957296907901764;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 24)) {
          sum += (float)0.012484082020819187;
        } else {
          sum += (float)-0.21694684028625488;
        }
      }
    }
  } else {
    if (!(data[1].missing != -1) || (data[1].qvalue < 8)) {
      if (!(data[3].missing != -1) || (data[3].qvalue < 40)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 44)) {
          sum += (float)0.14089375734329224;
        } else {
          sum += (float)-0.20241141319274902;
        }
      } else {
        if (!(data[1].missing != -1) || (data[1].qvalue < 0)) {
          sum += (float)0.046568434685468674;
        } else {
          sum += (float)-0.10277880728244781;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 30)) {
        if (!(data[1].missing != -1) || (data[1].qvalue < 34)) {
          sum += (float)0.028883621096611023;
        } else {
          sum += (float)-0.11764656752347946;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 16)) {
          sum += (float)-0.17736485600471497;
        } else {
          sum += (float)-0.22493939101696014;
        }
      }
    }
  }
  if (!(data[1].missing != -1) || (data[1].qvalue < 30)) {
    if (!(data[4].missing != -1) || (data[4].qvalue < 18)) {
      if (!(data[2].missing != -1) || (data[2].qvalue < 26)) {
        if (!(data[4].missing != -1) || (data[4].qvalue < 8)) {
          sum += (float)0.20314693450927734;
        } else {
          sum += (float)0.094855710864067078;
        }
      } else {
        if (!(data[0].missing != -1) || (data[0].qvalue < 28)) {
          sum += (float)0.15520292520523071;
        } else {
          sum += (float)0.017744883894920349;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 10)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 8)) {
          sum += (float)-0.068716421723365784;
        } else {
          sum += (float)0.1460193544626236;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 0)) {
          sum += (float)-0.22756186127662659;
        } else {
          sum += (float)-0.013574764132499695;
        }
      }
    }
  } else {
    if (!(data[0].missing != -1) || (data[0].qvalue < 6)) {
      if (!(data[3].missing != -1) || (data[3].qvalue < 42)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 40)) {
          sum += (float)0.13682496547698975;
        } else {
          sum += (float)-0.17183093726634979;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 30)) {
          sum += (float)0.053848262876272202;
        } else {
          sum += (float)-0.096521578729152679;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 22)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 6)) {
          sum += (float)-0.2065100222826004;
        } else {
          sum += (float)-0.043523166328668594;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 16)) {
          sum += (float)-0.14729741215705872;
        } else {
          sum += (float)-0.21388654410839081;
        }
      }
    }
  }
  if (!(data[0].missing != -1) || (data[0].qvalue < 14)) {
    if (!(data[4].missing != -1) || (data[4].qvalue < 24)) {
      if (!(data[1].missing != -1) || (data[1].qvalue < 16)) {
        if (!(data[4].missing != -1) || (data[4].qvalue < 0)) {
          sum += (float)-0.017262810841202736;
        } else {
          sum += (float)0.18502788245677948;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 18)) {
          sum += (float)0.15221469104290009;
        } else {
          sum += (float)0.028368089348077774;
        }
      }
    } else {
      if (!(data[2].missing != -1) || (data[2].qvalue < 38)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 4)) {
          sum += (float)-0.21373546123504639;
        } else {
          sum += (float)0.080926381051540375;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 0)) {
          sum += (float)0.31206592917442322;
        } else {
          sum += (float)-0.2223297655582428;
        }
      }
    }
  } else {
    if (!(data[1].missing != -1) || (data[1].qvalue < 4)) {
      if (!(data[3].missing != -1) || (data[3].qvalue < 32)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 46)) {
          sum += (float)0.12722808122634888;
        } else {
          sum += (float)-0.21527588367462158;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 28)) {
          sum += (float)0.055365212261676788;
        } else {
          sum += (float)-0.067839644849300385;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 34)) {
        if (!(data[4].missing != -1) || (data[4].qvalue < 6)) {
          sum += (float)-0.010691629722714424;
        } else {
          sum += (float)-0.12172114849090576;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 14)) {
          sum += (float)-0.12450658529996872;
        } else {
          sum += (float)-0.19436337053775787;
        }
      }
    }
  }
  if (!(data[0].missing != -1) || (data[0].qvalue < 30)) {
    if (!(data[4].missing != -1) || (data[4].qvalue < 18)) {
      if (!(data[2].missing != -1) || (data[2].qvalue < 26)) {
        if (!(data[4].missing != -1) || (data[4].qvalue < 2)) {
          sum += (float)0.19324116408824921;
        } else {
          sum += (float)0.10862087458372116;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 16)) {
          sum += (float)0.15795408189296722;
        } else {
          sum += (float)0.020903212949633598;
        }
      }
    } else {
      if (!(data[2].missing != -1) || (data[2].qvalue < 38)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 4)) {
          sum += (float)-0.18241980671882629;
        } else {
          sum += (float)0.06051105260848999;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 2)) {
          sum += (float)0.094938270747661591;
        } else {
          sum += (float)-0.19998271763324738;
        }
      }
    }
  } else {
    if (!(data[1].missing != -1) || (data[1].qvalue < 32)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 16)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 16)) {
          sum += (float)0.16632916033267975;
        } else {
          sum += (float)0.017866933718323708;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 34)) {
          sum += (float)-0.074593909084796906;
        } else {
          sum += (float)0.13145291805267334;
        }
      }
    } else {
      if (!(data[3].missing != -1) || (data[3].qvalue < 14)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 12)) {
          sum += (float)-0.16748015582561493;
        } else {
          sum += (float)-0.0077155143953859806;
        }
      } else {
        if (!(data[4].missing != -1) || (data[4].qvalue < 10)) {
          sum += (float)-0.11787798255681992;
        } else {
          sum += (float)-0.19412873685359955;
        }
      }
    }
  }
  if (!(data[1].missing != -1) || (data[1].qvalue < 10)) {
    if (!(data[2].missing != -1) || (data[2].qvalue < 4)) {
      if (!(data[4].missing != -1) || (data[4].qvalue < 32)) {
        if (!(data[3].missing != -1) || (data[3].qvalue < 4)) {
          sum += (float)0.26819157600402832;
        } else {
          sum += (float)-0.078942202031612396;
        }
      } else {
        if (!(data[3].missing != -1) || (data[3].qvalue < 2)) {
          sum += (float)0.0076199369505047798;
        } else {
          sum += (float)-0.21362960338592529;
        }
      }
    } else {
      if (!(data[2].missing != -1) || (data[2].qvalue < 34)) {
        if (!(data[0].missing != -1) || (data[0].qvalue < 10)) {
          sum += (float)0.14750032126903534;
        } else {
          sum += (float)0.055395312607288361;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 48)) {
          sum += (float)-0.027629494667053223;
        } else {
          sum += (float)-0.2000548392534256;
        }
      }
    }
  } else {
    if (!(data[3].missing != -1) || (data[3].qvalue < 32)) {
      if (!(data[0].missing != -1) || (data[0].qvalue < 36)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 6)) {
          sum += (float)-0.11037106812000275;
        } else {
          sum += (float)0.086572021245956421;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 36)) {
          sum += (float)-0.040204960852861404;
        } else {
          sum += (float)-0.19344706833362579;
        }
      }
    } else {
      if (!(data[0].missing != -1) || (data[0].qvalue < 2)) {
        if (!(data[2].missing != -1) || (data[2].qvalue < 30)) {
          sum += (float)0.065734297037124634;
        } else {
          sum += (float)-0.061069000512361526;
        }
      } else {
        if (!(data[2].missing != -1) || (data[2].qvalue < 14)) {
          sum += (float)-0.085965432226657867;
        } else {
          sum += (float)-0.17074288427829742;
        }
      }
    }
  }

  sum = sum + (float)(-0);
  if (!pred_margin) {
    return pred_transform(sum);
  } else {
    return sum;
  }
}

