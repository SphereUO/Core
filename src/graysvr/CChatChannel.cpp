
#include "CChatChannel.h"
#include "CChatChanMember.h"
#include "CChar.h"
#include "CClient.h"


CChatChannel::CChatChannel(LPCTSTR pszName, LPCTSTR pszPassword)
{
    m_sName = pszName;
    m_sPassword = pszPassword;
    m_fVoiceDefault = true;
}

CChatChannel* CChatChannel::GetNext() const
{
    return( static_cast <CChatChannel *>( CGObListRec::GetNext()));
}

LPCTSTR CChatChannel::GetName() const
{
    return( m_sName );
}

LPCTSTR CChatChannel::GetModeString() const
{
    // (client needs this) "0" = not passworded, "1" = passworded
    return(( IsPassworded()) ? "1" : "0" );
}

LPCTSTR CChatChannel::GetPassword() const
{
    return( m_sPassword );
}

void CChatChannel::SetPassword( LPCTSTR pszPassword)
{
    m_sPassword = pszPassword;
    return;
}

bool CChatChannel::IsPassworded() const
{
    return ( !m_sPassword.IsEmpty());
}

void CChatChannel::WhoIs(LPCTSTR pszBy, LPCTSTR pszMember)
{
    ADDTOCALLSTACK("CChatChannel::WhoIs");
    CChatChanMember * pBy = FindMember(pszBy);
    CChatChanMember * pMember = FindMember(pszMember);
    CChar * pChar = pMember? pMember->GetClient()->GetChar() : NULL;
    if (!pMember||!pChar)
    {
        pBy->SendChatMsg(CHATMSG_NoPlayer, pszMember);
    }
    else if (pMember->GetWhoIs())
    {
        pBy->SendChatMsg(CHATMSG_PlayerKnownAs, pszMember, pChar->GetName());
    }
    else
    {
        pBy->SendChatMsg(CHATMSG_PlayerIsAnonymous, pszMember);
    }
}

void CChatChannel::Emote(LPCTSTR pszBy, LPCTSTR pszMsg, CLanguageID lang )
{
    ADDTOCALLSTACK("CChatChannel::Emote");
    if (HasVoice(pszBy))
        Broadcast(CHATMSG_PlayerEmote, pszBy, pszMsg, lang );
    else
        FindMember(pszBy)->SendChatMsg(CHATMSG_RevokedSpeaking);
}

void CChatChannel::ToggleVoiceDefault(LPCTSTR  pszBy)
{
    ADDTOCALLSTACK("CChatChannel::ToggleVoiceDefault");
    if (!IsModerator(pszBy))
    {
        FindMember(pszBy)->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    if (GetVoiceDefault())
        Broadcast(CHATMSG_ModeratorsSpeakDefault, "", "");
    else
        Broadcast(CHATMSG_SpeakingByDefault, "", "");
    SetVoiceDefault(!GetVoiceDefault());
}

void CChatChannel::DisableVoiceDefault(LPCTSTR  pszBy)
{
    ADDTOCALLSTACK("CChatChannel::DisableVoiceDefault");
    if (GetVoiceDefault())
        ToggleVoiceDefault(pszBy);
}

void CChatChannel::EnableVoiceDefault(LPCTSTR  pszBy)
{
    ADDTOCALLSTACK("CChatChannel::EnableVoiceDefault");
    if (!GetVoiceDefault())
        ToggleVoiceDefault(pszBy);
}

void CChatChannel::SendPrivateMessage(CChatChanMember * pFrom, LPCTSTR pszTo, LPCTSTR pszMsg)
{
    ADDTOCALLSTACK("CChatChannel::SendPrivateMessage");
    CChatChanMember * pTo = FindMember(pszTo);
    if (!pTo)
    {
        pFrom->SendChatMsg(CHATMSG_NoPlayer, pszTo);
        return;
    }
    if (!pTo->IsReceivingAllowed())
    {
        pFrom->SendChatMsg(CHATMSG_PlayerNotReceivingPrivate, pszTo);
        return;
    }
    // Can always send private messages to moderators (but only if they are receiving)
    bool fHasVoice = HasVoice(pFrom->GetChatName());
    if ( !fHasVoice && !IsModerator(pszTo))
    {
        pFrom->SendChatMsg(CHATMSG_RevokedSpeaking);
        return;
    }

    if (pTo->IsIgnoring(pFrom->GetChatName())) // See if ignoring you
    {
        pFrom->SendChatMsg(CHATMSG_PlayerIsIgnoring, pszTo);
        return;
    }

    CGString sName;
    g_Serv.m_Chats.DecorateName(sName, pFrom);
    // Echo to the sending client so they know the message went out
    pFrom->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg);
    // If the sending and receiving are different send it out to the receiver
    if (pTo != pFrom)
        pTo->SendChatMsg(CHATMSG_PlayerPrivate, sName, pszMsg);
}

void CChatChannel::RenameChannel(CChatChanMember * pBy, LPCTSTR pszName)
{
    ADDTOCALLSTACK("CChatChannel::RenameChannel");
    // Ask the chat system if the new name is ok
    if ( ! g_Serv.m_Chats.IsValidName( pszName, false ))
    {
        pBy->SendChatMsg(CHATMSG_InvalidConferenceName);
        return;
    }
    // Ask the chat system if the new name is already taken
    if ( g_Serv.m_Chats.IsDuplicateChannelName(pszName))
    {
        pBy->SendChatMsg(CHATMSG_AlreadyAConference);
        return;
    }
    // Tell the channel members our name changed
    Broadcast(CHATMSG_ConferenceRenamed, GetName(), pszName);
    // Delete the old name from all chat clients
    g_Serv.m_Chats.SendDeleteChannel(this);
    // Do the actual renaming
    SetName(pszName);
    // Update all channel members' current channel bar
    Broadcast(CHATMSG_UpdateChannelBar, pszName, "");
    // Send out the new name to all chat clients so they can join
    g_Serv.m_Chats.SendNewChannel(this);
}

void CChatChannel::KickAll(CChatChanMember * pMemberException)
{
    ADDTOCALLSTACK("CChatChannel::KickAll");
    for (size_t i = 0; i < m_Members.GetCount(); i++)
    {
        if ( m_Members[i] == pMemberException) // If it's not me, then kick them
            continue;
        KickMember( pMemberException, m_Members[i] );
    }
}

void CChatChannel::RemoveMember(CChatChanMember * pMember)
{
    ADDTOCALLSTACK("CChatChannel::RemoveMember");
    for ( size_t i = 0; i < m_Members.GetCount(); )
    {
        // Tell the other clients in this channel (if any) you are leaving (including yourself)
        CClient * pClient = m_Members[i]->GetClient();

        if ( pClient == NULL )		//	auto-remove offline clients
        {
            m_Members[i]->SetChannel(NULL);
            m_Members.RemoveAt(i);
            continue;
        }

        pClient->addChatSystemMessage(CHATMSG_RemoveMember, pMember->GetChatName());
        if (m_Members[i] == pMember)	// disjoin
        {
            m_Members.RemoveAt(i);
            break;
        }

        i++;
    }

    // Update our persona
    pMember->SetChannel(NULL);
}

CChatChanMember * CChatChannel::FindMember(LPCTSTR pszName) const
{
    size_t i = FindMemberIndex( pszName );
    if ( i == m_Members.BadIndex() )
        return NULL;
    return m_Members[i];
}

bool CChatChannel::RemoveMember(LPCTSTR pszName)
{
    CChatChanMember * pMember = FindMember(pszName);
    if ( pMember == NULL )
        return false;
    RemoveMember(pMember);
    return true;
}

void CChatChannel::SetName(LPCTSTR pszName)
{
    m_sName = pszName;
}


bool CChatChannel::IsModerator(LPCTSTR pszMember) const
{
    ADDTOCALLSTACK("CChatChannel::IsModerator");
    for (size_t i = 0; i < m_Moderators.GetCount(); i++)
    {
        if (m_Moderators[i]->Compare(pszMember) == 0)
            return true;
    }
    return false;
}

bool CChatChannel::HasVoice(LPCTSTR pszMember) const
{
    ADDTOCALLSTACK("CChatChannel::HasVoice");
    for (size_t i = 0; i < m_NoVoices.GetCount(); i++)
    {
        if (m_NoVoices[i]->Compare(pszMember) == 0)
            return false;
    }
    return true;
}

void CChatChannel::SetModerator(LPCTSTR pszMember, bool fFlag)
{
    ADDTOCALLSTACK("CChatChannel::SetModerator");
    // See if they are already a moderator
    for (size_t i = 0; i < m_Moderators.GetCount(); i++)
    {
        if (m_Moderators[i]->Compare(pszMember) == 0)
        {
            if (fFlag == false)
                m_Moderators.DeleteAt(i);
            return;
        }
    }
    if (fFlag)
    {
        m_Moderators.Add( new CGString(pszMember) );
    }
}

void CChatChannel::KickMember(CChatChanMember *pByMember, CChatChanMember * pMember )
{
    ADDTOCALLSTACK("CChatChannel::KickMember");
    ASSERT( pMember );

    LPCTSTR pszByName;
    if (pByMember) // If NULL, then an ADMIN or a GM did it
    {
        pszByName = pByMember->GetChatName();
        if (!IsModerator(pszByName))
        {
            pByMember->SendChatMsg(CHATMSG_MustHaveOps);
            return;
        }
    }
    else
    {
        pszByName = "SYSTEM";
    }

    LPCTSTR pszName = pMember->GetChatName();

    // Kicking this person...remove from list of moderators first
    if (IsModerator(pszName))
    {
        SetModerator(pszName, false);
        SendThisMember(pMember);
        Broadcast(CHATMSG_PlayerNoLongerModerator, pszName, "");
        pMember->SendChatMsg(CHATMSG_RemovedListModerators, pszByName);
    }

    // Now kick them
    if (m_Members.GetCount() == 1) // If kicking yourself, send out to all clients in a chat that the channel is gone
        g_Serv.m_Chats.SendDeleteChannel(this);
    // Remove them from the channels list of members
    RemoveMember(pMember);
    // Tell the remain members about this
    Broadcast(CHATMSG_PlayerIsKicked, pszName, "");
    // Now clear their channel member list
    pMember->SendChatMsg(CHATMSG_ClearMemberList);
    // And give them the bad news
    pMember->SendChatMsg(CHATMSG_ModeratorHasKicked, pszByName);
}

bool CChatChannel::AddMember(CChatChanMember * pMember)
{
    ADDTOCALLSTACK("CChatChannel::AddMember");
    pMember->SetChannel(this);
    m_Members.Add( pMember );
    // See if only moderators have a voice by default
    LPCTSTR pszName = pMember->GetChatName();
    if (!GetVoiceDefault() && !IsModerator(pszName))
        // If only moderators have a voice by default, then add this member to the list of no voices
        SetVoice(pszName, false);
    // Set voice status
    return true;
}

void CChatChannel::SendMembers(CChatChanMember * pMember)
{
    ADDTOCALLSTACK("CChatChannel::SendMembers");
    for (size_t i = 0; i < m_Members.GetCount(); i++)
    {
        CGString sName;
        g_Serv.m_Chats.DecorateName(sName, m_Members[i]);
        pMember->SendChatMsg(CHATMSG_SendPlayerName, sName);
    }
}

void CChatChannel::SendThisMember(CChatChanMember * pMember, CChatChanMember * pToMember)
{
    ADDTOCALLSTACK("CChatChannel::SendThisMember");
    TCHAR buffer[2048];
    sprintf(buffer, "%s%s",
            (IsModerator(pMember->GetChatName()) == true) ? "1" :
            (HasVoice(pMember->GetChatName()) == true) ? "0" : "2", pMember->GetChatName());
    // If no particular member is specified in pToMember, then send it out to all members
    if (pToMember == NULL)
    {
        for (size_t i = 0; i < m_Members.GetCount(); i++)
        {
            // Don't send out members if they are ignored by someone
            if (!m_Members[i]->IsIgnoring(pMember->GetChatName()))
                m_Members[i]->SendChatMsg(CHATMSG_SendPlayerName, buffer);
        }
    }
    else
    {
        // Don't send out members if they are ignored by someone
        if (!pToMember->IsIgnoring(pMember->GetChatName()))
            pToMember->SendChatMsg(CHATMSG_SendPlayerName, buffer);
    }
}

void CChatChannel::SetVoice(LPCTSTR pszName, bool fFlag)
{
    ADDTOCALLSTACK("CChatChannel::SetVoice");
    // See if they have no voice already
    for (size_t i = 0; i < m_NoVoices.GetCount(); i++)
    {
        if (m_NoVoices[i]->Compare(pszName) == 0)
        {
            if (fFlag == true)
                m_NoVoices.DeleteAt(i);
            return;
        }
    }
    if (fFlag == false)
    {
        m_NoVoices.Add( new CGString(pszName) );
        return;
    }
}

void CChatChannel::MemberTalk(CChatChanMember * pByMember, LPCTSTR pszText, CLanguageID lang )
{
    ADDTOCALLSTACK("CChatChannel::MemberTalk");
    // Do I have a voice?
    if (!HasVoice(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_RevokedSpeaking);
        return;
    }
    Broadcast(CHATMSG_PlayerTalk, pByMember->GetChatName(), pszText, lang );
}

void CChatChannel::ChangePassword(CChatChanMember * pByMember, LPCTSTR pszPassword)
{
    ADDTOCALLSTACK("CChatChannel::ChangePassword");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->GetClient()->addChatSystemMessage(CHATMSG_MustHaveOps);
    }
    else
    {
        SetPassword(pszPassword);
        g_Serv.m_Chats.SendNewChannel(pByMember->GetChannel());
        Broadcast(CHATMSG_PasswordChanged, "","");
    }
}

void CChatChannel::Broadcast(CHATMSG_TYPE iType, LPCTSTR pszName, LPCTSTR pszText, CLanguageID lang, bool fOverride )
{
    ADDTOCALLSTACK("CChatChannel::Broadcast");
    CGString sName;
    CChatChanMember *pSendingMember = FindMember(pszName);

    if (iType >= CHATMSG_PlayerTalk && iType <= CHATMSG_PlayerPrivate) // Only chat, emote, and privates get a color status number
        g_Serv.m_Chats.DecorateName(sName, pSendingMember, fOverride);
    else
        sName = pszName;

    for (size_t i = 0; i < m_Members.GetCount(); i++)
    {
        // Check to see if the recipient is ignoring messages from the sender
        // Just pass over it if it's a regular talk message
        if (!m_Members[i]->IsIgnoring(pszName))
        {
            m_Members[i]->SendChatMsg(iType, sName, pszText, lang );
        }

            // If it's a private message, then tell the sender the recipient is ignoring them
        else if (iType == CHATMSG_PlayerPrivate)
        {
            pSendingMember->SendChatMsg(CHATMSG_PlayerIsIgnoring, m_Members[i]->GetChatName());
        }
    }
}

void CChatChannel::GrantVoice(CChatChanMember * pByMember, LPCTSTR pszName)
{
    ADDTOCALLSTACK("CChatChannel::GrantVoice");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (HasVoice(pszName))
        return;
    SetVoice(pszName, true);
    SendThisMember(pMember); // Update the color
    pMember->SendChatMsg(CHATMSG_ModeratorGrantSpeaking, pByMember->GetChatName());
    Broadcast(CHATMSG_PlayerNowSpeaking, pszName, "", "");
}

void CChatChannel::RevokeVoice(CChatChanMember * pByMember, LPCTSTR pszName)
{
    ADDTOCALLSTACK("CChatChannel::RevokeVoice");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (!HasVoice(pszName))
        return;
    SetVoice(pszName, false);
    SendThisMember(pMember); // Update the color
    pMember->SendChatMsg(CHATMSG_ModeratorRemovedSpeaking, pByMember->GetChatName());
    Broadcast(CHATMSG_PlayerNoSpeaking, pszName, "", "");
}

void CChatChannel::ToggleVoice(CChatChanMember * pByMember, LPCTSTR pszName)
{
    ADDTOCALLSTACK("CChatChannel::ToggleVoice");
    if (!HasVoice(pszName)) // (This also returns true if this person is not in the channel)
        GrantVoice(pByMember, pszName); // this checks and reports on membership
    else
        RevokeVoice(pByMember, pszName); // this checks and reports on membership
}

size_t CChatChannel::FindMemberIndex(LPCTSTR pszName) const
{
    ADDTOCALLSTACK("CChatChannel::FindMemberIndex");
    for (size_t i = 0; i < m_Members.GetCount(); i++)
    {
        if ( strcmp( m_Members[i]->GetChatName(), pszName) == 0)
            return i;
    }
    return m_Members.BadIndex();
}

void CChatChannel::GrantModerator(CChatChanMember * pByMember, LPCTSTR pszName)
{
    ADDTOCALLSTACK("CChatChannel::GrantModerator");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (IsModerator(pMember->GetChatName()))
        return;
    SetModerator(pszName, true);
    SendThisMember(pMember); // Update the color
    Broadcast(CHATMSG_PlayerIsAModerator, pMember->GetChatName(), "", "");
    pMember->SendChatMsg(CHATMSG_YouAreAModerator, pByMember->GetChatName());
}

void CChatChannel::RevokeModerator(CChatChanMember * pByMember, LPCTSTR pszName)
{
    ADDTOCALLSTACK("CChatChannel::RevokeModerator");
    if (!IsModerator(pByMember->GetChatName()))
    {
        pByMember->SendChatMsg(CHATMSG_MustHaveOps);
        return;
    }
    CChatChanMember * pMember = FindMember(pszName);
    if (!pMember)
    {
        pByMember->SendChatMsg(CHATMSG_NoPlayer, pszName);
        return;
    }
    if (!IsModerator(pMember->GetChatName()))
        return;
    SetModerator(pszName, false);
    SendThisMember(pMember); // Update the color
    Broadcast(CHATMSG_PlayerNoLongerModerator, pMember->GetChatName(), "", "");
    pMember->SendChatMsg(CHATMSG_RemovedListModerators, pByMember->GetChatName());
}

void CChatChannel::ToggleModerator(CChatChanMember * pByMember, LPCTSTR pszName)
{
    ADDTOCALLSTACK("CChatChannel::ToggleModerator");
    if (!IsModerator(pszName))
        GrantModerator(pByMember, pszName);
    else
        RevokeModerator(pByMember, pszName);
}

bool CChatChannel::GetVoiceDefault()  const
{
    return m_fVoiceDefault;
}

void CChatChannel::SetVoiceDefault(bool fVoiceDefault)
{
    m_fVoiceDefault = fVoiceDefault;
}