#include "HUDSoundManager.h"


namespace HUD
{
    // Unleashed HUD shit

    //static float sMusicVolume = 1.0f;

    // TODO: move somewhere else
    struct CSoundPlayerCri
    {
        void* vftable /*VFT*/;
        int m_Field004;
        int m_Field008;
        int m_Field00C;
        int m_Field010;
        int m_Field014;
        int m_Field018;
        float m_Volume;
    };

    HOOK(void, __stdcall, _HudPreparePause, 0x10A1140, void* in_Hud)
    {
        // Todo: fade music over time

        Sonic::CGameDocument* gameDocument = Sonic::CGameDocument::GetInstance().get().get();
        hh::vector<Sonic::SBGMData>* audioData = &gameDocument->m_pMember->m_AudioData;

        for (int i = 0; i < audioData->size(); ++i)
        {
            Hedgehog::Sound::CSoundHandleBgm* bgm = audioData->at(i).spSoundHandleBGM.get();

            CSoundPlayerCri* soundPlayer = (CSoundPlayerCri*)bgm->GetSoundPlayer();
            SoundManager::s_StartingVolume = soundPlayer->m_Volume;

            FUNCTION_PTR(void*, __thiscall, SoundPlayerSetVolume, 0x00765850, void* _SoundPlayer, float _Volume);
            SoundPlayerSetVolume(soundPlayer, SoundManager::s_StartingVolume * 0.25f);
        }

        original_HudPreparePause(in_Hud);
    }
    HOOK(void, __stdcall, _HudPrepareUnPause, 0x010A1480, void* in_Hud)
    {
        // Todo: fade music over time

        Sonic::CGameDocument* gameDocument = Sonic::CGameDocument::GetInstance().get().get();
        hh::vector<Sonic::SBGMData>* audioData = &gameDocument->m_pMember->m_AudioData;

        for (int i = 0; i < audioData->size(); ++i)
        {
            Hedgehog::Sound::CSoundHandleBgm* bgm = audioData->at(i).spSoundHandleBGM.get();

            CSoundPlayerCri* soundPlayer = (CSoundPlayerCri*)bgm->GetSoundPlayer();

            FUNCTION_PTR(void*, __thiscall, SoundPlayerSetVolume, 0x00765850, void* _SoundPlayer, float _Volume);
            SoundPlayerSetVolume(soundPlayer, SoundManager::s_StartingVolume);
        }

        original_HudPrepareUnPause(in_Hud);
    }
}


void HUD::SoundManager::Init()
{
    INSTALL_HOOK(_HudPreparePause)
    INSTALL_HOOK(_HudPrepareUnPause)
    WRITE_NOP(0x010A128D, 0x010A1292 - 0x010A128D); // Nop music pause
    //WRITE_NOP(0x010A1806, 0x010A180B - 0x010A1806); // Nop music unpause -- unnecessary, but should be fine? disable for now
}

void HUD::SoundManager::OnFrame()
{
	//if 
}
