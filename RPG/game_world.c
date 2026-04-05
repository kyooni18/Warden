#include "game_shared.h"

const ZoneData kZones[ZONE_COUNT] = {
    [ZONE_EMBERFALL_GATE] = {
        .name = "엠버폴 관문",
        .short_name = "관문",
        .description =
            "현무암 관문이 옛 왕국 가도를 지키고 있습니다. 보급 화로는 밤새 타오르고, "
            "남쪽으로 향하는 모든 여행자는 잿빛 벌판 너머에서 죽은 궁정이 깨어난다는 "
            "소문을 속삭입니다.",
        .scout_text =
            "새로운 군화 자국, 병참 깃발, 안전한 야영지. 더 깊이 들어가기 전에 "
            "재정비하기 가장 좋은 곳입니다.",
        .npc = "병참장교 아이븐",
        .resource = RESOURCE_NONE,
        .danger = 0,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_BRASS_MARKET] = {
        .name = "황동 시장",
        .short_name = "시장",
        .description =
            "기워 붙인 천막 아래 시장이 간신히 버티고 있습니다. 등불과 흥정이 가득한 "
            "곳으로, 밀수꾼·유물 상인·상단 경비가 동전만큼 빠르게 소문을 주고받습니다.",
        .scout_text =
            "혼잡하고 시끄럽지만 쓸모 있는 곳입니다. 포션이 빠르게 팔려 나가고, 겁먹은 "
            "상인들은 계속 철목 고개 쪽을 가리킵니다.",
        .npc = "상인 살",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = true,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_LANTERN_WARD] = {
        .name = "등불 구역",
        .short_name = "등구",
        .description =
            "줄지은 감시탑과 좁은 골목이 잠들지 않는 구역을 둘러싸고 있습니다. 전령은 "
            "봉인된 명령서를 나르고, 기록관들은 낡은 지도를 따라갈 용감한 이를 찾습니다.",
        .scout_text =
            "구역은 안전하지만 모든 망루가 남쪽 길을 주시하고 있습니다. 전령들은 이동 "
            "상단 야영지와 절벽 쪽의 기이한 날씨를 언급합니다.",
        .npc = "전령 나라",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_WHISPER_LIBRARY] = {
        .name = "속삭임 도서관",
        .short_name = "도관",
        .description =
            "부러진 갈비뼈처럼 기울어진 서가가 차가운 관측 돔을 에워쌉니다. 도서관엔 "
            "여전히 결계 문양의 울림이 남아 있고, 모든 탁자는 침수 기록고·빙설 길·"
            "분화구 가장자리 지도로 덮여 있습니다.",
        .scout_text =
            "안전한 벽, 위태로운 지식. 기록관들은 유물 파편을 해독할 수 있지만, 먼저 "
            "가져와 줄 사람이 필요합니다.",
        .npc = "기록관 센",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = true,
    },
    [ZONE_IRONWOOD_PASS] = {
        .name = "철목 고개",
        .short_name = "철목",
        .description =
            "검은 소나무가 붉은 암석을 가른 가파른 길 주변에 얽혀 있습니다. 고개에는 "
            "수레 바퀴 소리, 도적의 휘파람, 벼락에 갈라진 고목의 파열음이 메아리칩니다.",
        .scout_text =
            "도적은 능선을 장악하고 늑대는 낮은 수풀을 맴돕니다. 도로 절개면에는 풍부한 "
            "광맥이 드러나 있습니다.",
        .npc = "이곳에는 믿을 만한 동맹이 없습니다.",
        .resource = RESOURCE_ORE,
        .danger = 2,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_VERDANT_ABBEY] = {
        .name = "청록 수도원",
        .short_name = "수원",
        .description =
            "반쯤 무너진 수도원에는 약초 담쟁이와 월광화가 무성합니다. 예배당 종은 "
            "사라졌지만 수녀들은 여전히 갑옷을 고치고 상처를 꿰매며 죽어가는 시골의 "
            "기록을 남깁니다.",
        .scout_text =
            "남부에 남은 가장 온화한 장소입니다. 수녀들은 절박한 이들을 치료하며, 이를 "
            "이어가기 위해 늪의 약초가 필요합니다.",
        .npc = "수녀 엘로웬",
        .resource = RESOURCE_NONE,
        .danger = 1,
        .safe = true,
        .merchant = false,
        .healer = true,
        .forge = false,
        .archive = false,
    },
    [ZONE_MOONFEN] = {
        .name = "월광 늪",
        .short_name = "월늪",
        .description =
            "흘러다니는 등불 버섯 아래로 은빛 갈대와 검은 물이 펼쳐집니다. 멀리서 보면 "
            "평온하지만, 눈이 지나치게 많은 무언가가 수면 아래서 움직이기 전까지만 "
            "그렇습니다.",
        .scout_text =
            "월엽초가 무성하지만 거머리와 늪 마녀도 함께 번성합니다. 용기보다 신중한 "
            "발걸음이 더 오래 살게 해줍니다.",
        .npc = "늪만이 메아리칩니다.",
        .resource = RESOURCE_HERB,
        .danger = 3,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_STORMWATCH_CLIFFS] = {
        .name = "폭풍감시 절벽",
        .short_name = "절벽",
        .description =
            "난삽한 절벽이 회색 바다 위로 솟아 있습니다. 난파선 조각이 파도에 휩쓸리고, "
            "감시 초소 사이로 바람 깃발이 펄럭입니다. 모든 바위턱이 한 번의 돌풍만으로 "
            "무덤이 될 듯합니다.",
        .scout_text =
            "민첩한 약탈자들이 이곳 바위턱에서 기습합니다. 바람이 돌아서면 절벽은 사각지대와 "
            "비명 같은 바람으로 가득한 시련장이 됩니다.",
        .npc = "버려진 봉화만 타닥거립니다.",
        .resource = RESOURCE_NONE,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_ASHEN_QUARRY] = {
        .name = "잿빛 채석장",
        .short_name = "채석",
        .description =
            "채석장 바닥엔 오래된 불빛이 남아 있습니다. 부서진 석판 위로 체인 리프트가 "
            "매달려 있고, 버려진 대장간엔 적절한 광석만 있다면 새 강철을 단련할 열이 "
            "아직 남아 있습니다.",
        .scout_text =
            "좋은 광석, 나쁜 동행. 채석장 망자와 슬래그 괴수가 구덩이를 배회하지만, "
            "숙련된 손이라면 낡은 모루를 다시 쓸 수 있습니다.",
        .npc = "대장간은 아직 사용할 수 있습니다.",
        .resource = RESOURCE_ORE,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = true,
        .archive = false,
    },
    [ZONE_SUNKEN_ARCHIVE] = {
        .name = "침수 기록고",
        .short_name = "기록",
        .description =
            "수십 년 전 도서 지구가 조수 분지로 무너져 내렸습니다. 기울어진 서가 사이 "
            "대리석 계단은 물속으로 사라지고, 익사한 서기관들이 수면 아래 목록 회랑을 "
            "여전히 순찰합니다.",
        .scout_text =
            "수면 아래 오래된 결계가 깜박입니다. 유물 파편이 남아 있다면 아래의 "
            "큐레이터 금고에 있을 가능성이 큽니다.",
        .npc = "조수만이 시간을 셉니다.",
        .resource = RESOURCE_NONE,
        .danger = 5,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = true,
    },
    [ZONE_GLOAM_PORT] = {
        .name = "황혼 항구",
        .short_name = "항구",
        .description =
            "더 이상 배가 떠나지 않지만 부두에는 중개인, 뱃사공, 수집꾼이 등불과 갈고리로 "
            "일을 이어갑니다. 항만장은 말보다 더 많은 것을 알고 있습니다.",
        .scout_text =
            "여기서는 아직 보급품을 살 수 있고, 노선에 익숙한 선장들은 빙첨로로 가는 길을 "
            "어떤 지도보다 정확히 압니다.",
        .npc = "미렐 선장",
        .resource = RESOURCE_NONE,
        .danger = 2,
        .safe = true,
        .merchant = true,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_FROSTSPIRE_TRAIL] = {
        .name = "빙첨로",
        .short_name = "빙첨",
        .description =
            "부서진 석상 아래 길은 얼어붙은 선반 지형으로 좁아집니다. 이곳의 눈은 녹지 "
            "않고, 산의 망자를 피하려다 쓰러진 모든 상단의 뼈 위에 층층이 쌓입니다.",
        .scout_text =
            "이 길의 고요함은 가장 불길한 종류입니다. 망령이 눈 아래 숨어 있고, 추위 "
            "자체가 포식자처럼 물어뜯습니다.",
        .npc = "산바람은 모든 질문에 이빨로 답합니다.",
        .resource = RESOURCE_NONE,
        .danger = 6,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_CINDER_GROVE] = {
        .name = "숯불 수림",
        .short_name = "수림",
        .description =
            "그을린 나무줄기 사이에서도 이상하게 꽃이 피는 숲입니다. 흙은 따뜻하고 공기엔 "
            "삼나무와 재 냄새가 섞여 있으며, 폭풍 뒤마다 희귀 약초가 그을음 사이로 "
            "돋아납니다.",
        .scout_text =
            "이 숲은 신중한 채집에 보답합니다. 날씨가 바뀌면 불사슴과 가시주술사가 연기 "
            "속에 숨어듭니다.",
        .npc = "수피엔 뿔자국만 남아 있습니다.",
        .resource = RESOURCE_HERB,
        .danger = 4,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_OBSIDIAN_CRATER] = {
        .name = "흑요 분화구",
        .short_name = "분화",
        .description =
            "검은 유리질 지면과 붉은 증기가 뒤엉킨 거대한 충돌 분지입니다. 지각 아래 "
            "용암맥이 맥박치고, 광신도들은 여전히 분화구 가장자리를 땅속 무언가를 위한 "
            "성당처럼 떠받듭니다.",
        .scout_text =
            "강한 광석과 더 강한 적이 있는 곳입니다. 유물 파편을 품을 만큼 강한 존재라면 "
            "불꽃과 파편 속에서 이곳에서 깨어날 것입니다.",
        .npc = "분화구가 스스로에게 주문을 외웁니다.",
        .resource = RESOURCE_ORE,
        .danger = 7,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_RUINED_BASILICA] = {
        .name = "폐허 대성당",
        .short_name = "성당",
        .description =
            "한때 황금빛이던 대성당은 이제 지붕 없이 어두운 하늘 아래 서 있습니다. "
            "기도 모자이크는 파편으로만 남았고, 중앙 회랑은 왕관이 깨어나는 매몰 왕좌를 "
            "향해 화살처럼 뻗어 있습니다.",
        .scout_text =
            "대성당은 경계선입니다. 영혼들이 이곳에 모이고, 여기서 얻어낼 수 있는 축복은 "
            "왕좌 앞에서 큰 의미를 가질 수 있습니다.",
        .npc = "메아리가 부서진 전례문을 읊습니다.",
        .resource = RESOURCE_NONE,
        .danger = 6,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_HOLLOW_THRONE] = {
        .name = "공허 왕좌",
        .short_name = "왕좌",
        .description =
            "묻힌 궁전은 돌뿌리와 금 간 대리석으로 된 거대한 성소로 이어집니다. 중심엔 "
            "그림자에 감싼 빈 왕좌가 있고, 왕국의 마지막 굶주림이 자신의 이름을 되찾으려 "
            "버둥거립니다.",
        .scout_text =
            "이곳의 모든 것은 여명의 열쇠를, 당신의 목숨을, 혹은 그 둘 모두를 원합니다.",
        .npc = "이곳엔 필멸의 존재가 기다리지 않습니다.",
        .resource = RESOURCE_NONE,
        .danger = 8,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_DEEPWOOD_HOLLOW] = {
        .name = "심숲 분지",
        .short_name = "심숲",
        .description =
            "숯불 수림 남쪽, 고대의 나무가 하늘을 가리는 분지입니다. 수천 년 된 고목 사이로 "
            "드루이드의 제단과 이끼 낀 석상이 서 있으며, 공기는 두꺼운 생명의 향기로 가득 "
            "합니다. 이곳의 정령들은 아직도 살아 숨쉽니다.",
        .scout_text =
            "드루이드 에이브가 성소를 지킵니다. 희귀한 약초가 지천에 자라지만, 숲의 "
            "수호령과 가시 정령이 침입자를 달가워하지 않습니다.",
        .npc = "드루이드 에이브",
        .resource = RESOURCE_HERB,
        .danger = 5,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = false,
    },
    [ZONE_MAGMA_RIFT] = {
        .name = "용암 균열",
        .short_name = "용균",
        .description =
            "흑요 분화구 남쪽, 지각이 갈라지며 드러난 거대한 용암 지대입니다. 붉은 용암이 "
            "갈라진 암반 틈 사이로 끊임없이 흘러내리고, 공기는 황 냄새와 열기로 숨이 막힙니다. "
            "극한 환경이지만 가장 단단한 광석이 이곳에서만 나옵니다.",
        .scout_text =
            "용암 균열은 최후의 시험장입니다. 용암 골렘과 화염 폭군이 배회하며, 극소수의 "
            "단련된 대장장이만이 이곳의 불꽃 앞에서 최고의 강철을 뽑아냅니다.",
        .npc = "용암만이 노래합니다.",
        .resource = RESOURCE_ORE,
        .danger = 8,
        .safe = false,
        .merchant = false,
        .healer = false,
        .forge = true,
        .archive = false,
    },
    [ZONE_ANCIENT_BEACON] = {
        .name = "고대 봉화",
        .short_name = "봉화",
        .description =
            "폐허 대성당 남쪽 절벽 위에 자리한 오래된 봉화대입니다. 한때 왕국 전역에 "
            "신호를 보내던 이 봉화는 오래전 꺼졌습니다. 봉화지기 오른이 혼자 이곳을 지키며 "
            "봉화를 다시 밝히려는 희망을 잃지 않고 있습니다.",
        .scout_text =
            "봉화지기 오른은 친절하지만 지쳐 있습니다. 봉화를 다시 밝히면 왕국 전역에 "
            "희망의 빛을 보낼 수 있습니다. 이곳은 비교적 안전하지만 길 잃은 병사들이 "
            "때로 이곳까지 헤매어 옵니다.",
        .npc = "봉화지기 오른",
        .resource = RESOURCE_NONE,
        .danger = 3,
        .safe = true,
        .merchant = false,
        .healer = true,
        .forge = false,
        .archive = false,
    },
    [ZONE_SHATTERED_VAULT] = {
        .name = "파쇄 금고",
        .short_name = "금고",
        .description =
            "공허 왕좌 아래 깊이 묻혀 있던 왕국의 마지막 비밀 금고입니다. 왕관이 부서지자 "
            "봉인이 풀리며 이 공간이 드러났습니다. 수백 년간 잠들어 있던 보물과 고대 기록이 "
            "먼지 속에 잠들어 있습니다.",
        .scout_text =
            "왕관의 사슬에서 해방된 공간입니다. 위험은 없지만, 이곳에는 왕국의 진실이 "
            "담긴 기록과 그 어느 곳에서도 찾을 수 없는 보물이 잠들어 있습니다.",
        .npc = "오래된 침묵만이 흐릅니다.",
        .resource = RESOURCE_NONE,
        .danger = 0,
        .safe = true,
        .merchant = false,
        .healer = false,
        .forge = false,
        .archive = true,
    },
};
const char *kWeatherNames[WEATHER_COUNT] = {
    "맑음", "비", "안개", "강풍", "화산재", "폭풍"};
const char *kFragmentNames[FRAGMENT_COUNT] = {
    "조수 파편", "서리 파편", "불씨 파편"};
