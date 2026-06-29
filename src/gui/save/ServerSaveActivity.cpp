#include "ServerSaveActivity.h"
#include "graphics/Graphics.h"
#include "graphics/VideoBuffer.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/Button.h"
#include "gui/interface/Checkbox.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/dialogues/SaveIDMessage.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/InformationMessage.h"
#include "client/Client.h"
#include "client/ThumbnailRendererTask.h"
#include "client/GameSave.h"
#include "client/http/UploadSaveRequest.h"
#include "tasks/Task.h"
#include "gui/Style.h"

class SaveUploadTask: public Task
{
	SaveInfo &save;

	void before() override
	{

	}

	void after() override
	{

	}

	bool doWork() override
	{
		notifyProgress(-1);
		auto uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(save);
		uploadSaveRequest->Start();
		uploadSaveRequest->Wait();
		try
		{
			save.SetID(uploadSaveRequest->Finish());
		}
		catch (const http::RequestError &ex)
		{
			notifyError(ByteString(ex.what()).FromUtf8());
			return false;
		}
		return true;
	}

public:
	SaveUploadTask(SaveInfo &newSave):
		save(newSave)
	{

	}
};

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(440, 200)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	titleLabel = new ui::Label(ui::Point(4, 5), ui::Point((Size.X/2)-8, 16), "");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);
	CheckName(save->GetName()); //set titleLabel text

	ui::Label * previewLabel = new ui::Label(ui::Point((Size.X/2)+4, 5), ui::Point((Size.X/2)-8, 16), "Предпросмотр:");
	previewLabel->SetTextColour(style::Colour::InformationTitle);
	previewLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	previewLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(previewLabel);

	nameField = new ui::Textbox(ui::Point(8, 25), ui::Point((Size.X/2)-16, 16), save->GetName(), "[имя сохранения]");
	nameField->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	nameField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	nameField->SetActionCallback({ [this] { CheckName(nameField->GetText()); } });
	nameField->SetLimit(50);
	AddComponent(nameField);
	FocusComponent(nameField);

	descriptionField = new ui::Textbox(ui::Point(8, 65), ui::Point((Size.X/2)-16, Size.Y-(65+16+4)), save->GetDescription(), "[описание сохранения]");
	descriptionField->SetMultiline(true);
	descriptionField->SetLimit(254);
	descriptionField->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	descriptionField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	AddComponent(descriptionField);

	publishedCheckbox = new ui::Checkbox(ui::Point(8, 45), ui::Point((Size.X/2)-80, 16), "Опубликовать", "");
	auto user = Client::Ref().GetAuthUser();
	if (!(user && user->Username == save->GetUserName()))
	{
		//Save is not owned by the user, disable by default
		publishedCheckbox->SetChecked(false);
	}
	else
	{
		//Save belongs to the current user, use published state already set
		publishedCheckbox->SetChecked(save->GetPublished());
	}
	AddComponent(publishedCheckbox);

	pausedCheckbox = new ui::Checkbox(ui::Point(160, 45), ui::Point(55, 16), "На паузе", "");
	pausedCheckbox->SetChecked(save->GetGameSave()->paused);
	AddComponent(pausedCheckbox);

	ui::Button * cancelButton = new ui::Button(ui::Point(0, Size.Y-16), ui::Point((Size.X/2)-75, 16), "Отмена");
	cancelButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	cancelButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	cancelButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	cancelButton->SetActionCallback({ [this] {
		Exit();
	} });
	AddComponent(cancelButton);
	SetCancelButton(cancelButton);

	ui::Button * okayButton = new ui::Button(ui::Point((Size.X/2)-76, Size.Y-16), ui::Point(76, 16), "Сохранить");
	okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	okayButton->Appearance.TextInactive = style::Colour::InformationTitle;
	okayButton->SetActionCallback({ [this] {
		Save();
	} });
	AddComponent(okayButton);
	SetOkayButton(okayButton);

	ui::Button * PublishingInfoButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-42), ui::Point(150, 16), "О публикации");
	PublishingInfoButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	PublishingInfoButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	PublishingInfoButton->Appearance.TextInactive = style::Colour::InformationTitle;
	PublishingInfoButton->SetActionCallback({ [this] {
		ShowPublishingInfo();
	} });
	AddComponent(PublishingInfoButton);

	ui::Button * RulesButton = new ui::Button(ui::Point((Size.X*3/4)-75, Size.Y-22), ui::Point(150, 16), "Правила загрузки");
	RulesButton->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	RulesButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	RulesButton->Appearance.TextInactive = style::Colour::InformationTitle;
	RulesButton->SetActionCallback({ [this] {
		ShowRules();
	} });
	AddComponent(RulesButton);

	if (save->GetGameSave())
	{
		thumbnailRenderer = new ThumbnailRendererTask(*save->GetGameSave(), Size / 2 - Vec2(16, 16), RendererSettings::decorationAntiClickbait, true);
		thumbnailRenderer->Start();
	}
}

ServerSaveActivity::ServerSaveActivity(std::unique_ptr<SaveInfo> newSave, bool saveNow, OnUploaded onUploaded_) :
	WindowActivity(ui::Point(-1, -1), ui::Point(200, 50)),
	thumbnailRenderer(nullptr),
	save(std::move(newSave)),
	onUploaded(onUploaded_),
	saveUploadTask(nullptr)
{
	ui::Label * titleLabel = new ui::Label(ui::Point(0, 0), Size, "Сохранение на сервер...");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignCentre;
	titleLabel->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	AddComponent(titleLabel);

	AddAuthorInfo();

	saveUploadTask = new SaveUploadTask(*this->save);
	saveUploadTask->AddTaskListener(this);
	saveUploadTask->Start();
}

void ServerSaveActivity::NotifyDone(Task * task)
{
	if(!task->GetSuccess())
	{
		Exit();
		new ErrorMessage("Ошибка", task->GetError());
	}
	else
	{
		if (onUploaded)
		{
			onUploaded(std::move(save));
		}
		Exit();
	}
}

void ServerSaveActivity::Save()
{
	if (!nameField->GetText().length())
	{
		new ErrorMessage("Ошибка", "Вы должны указать имя сохранения.");
		return;
	}
	auto user = Client::Ref().GetAuthUser();
	if (!(user && user->Username == save->GetUserName()) && publishedCheckbox->GetChecked())
	{
		new ConfirmPrompt("Опубликовать", "Это сохранение создано пользователем " + save->GetUserName().FromUtf8() + "; вы собираетесь опубликовать его под своим именем. Если автор не дал вам разрешения, снимите отметку «Опубликовать», иначе продолжите", { [this] {
			saveUpload();
		} });
	}
	else
	{
		saveUpload();
	}
}

void ServerSaveActivity::AddAuthorInfo()
{
	Bson serverSaveInfo;
	serverSaveInfo["type"] = "save";
	serverSaveInfo["id"] = save->GetID();
	auto user = Client::Ref().GetAuthUser();
	serverSaveInfo["username"] = user ? user->Username : ByteString("");
	serverSaveInfo["title"] = save->GetName().ToUtf8();
	serverSaveInfo["description"] = save->GetDescription().ToUtf8();
	serverSaveInfo["published"] = (int)save->GetPublished();
	serverSaveInfo["date"] = int64_t(time(nullptr));
	Client::Ref().SaveAuthorInfo(serverSaveInfo);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->authors = serverSaveInfo;
		save->SetGameSave(std::move(gameSave));
	}
}

void ServerSaveActivity::saveUpload()
{
	okayButton->Enabled = false;
	save->SetName(nameField->GetText());
	save->SetDescription(descriptionField->GetText());
	save->SetPublished(publishedCheckbox->GetChecked());
	auto user = Client::Ref().GetAuthUser();
	save->SetUserName(user ? user->Username : ByteString(""));
	save->SetID(0);
	{
		auto gameSave = save->TakeGameSave();
		gameSave->paused = pausedCheckbox->GetChecked();
		save->SetGameSave(std::move(gameSave));
	}
	AddAuthorInfo();
	uploadSaveRequest = std::make_unique<http::UploadSaveRequest>(*save);
	uploadSaveRequest->Start();
}

void ServerSaveActivity::Exit()
{
	WindowActivity::Exit();
}

void ServerSaveActivity::ShowPublishingInfo()
{
	String info =
		"В The Powder Toy симуляции можно сохранять в аккаунт с двумя уровнями приватности: опубликованные и неопубликованные. Выберите нужный вариант, отмечая или снимая отметку в чекбоксе «Опубликовать». По умолчанию сохранения неопубликованы — если не отметить «Опубликовать», никто не сможет видеть ваши сохранения.\n"
		"\n"
		"\btОпубликованные сохранения\bw появляются в ленте «По дате» и видны многим людям. Они также влияют на ваш средний балл, который отображается на странице профиля на сайте. Опубликуйте сохранения, которые хотите показать людям, чтобы они могли комментировать и голосовать.\n"
		"\btНеопубликованные сохранения\bw не показываются в ленте «По дате» и не влияют на средний балл. Они не полностью приватны: любой, кто знает id сохранения, может его открыть. Вы можете передать id конкретным людям, чтобы показать им сохранение, но не всем.\n"
		"\n"
		"Чтобы быстро пересохранить, откройте сохранение и нажмите левую часть раздельной кнопки пересохранения — \bt«Перезагрузить текущую симуляцию»\bw. Чтобы изменить описание или статус публикации, нажмите правую часть — \bt«Изменить свойства симуляции»\bw. Имя сохранения изменить нельзя; это создаст совершенно новое сохранение без комментариев, голосов и тегов, отдельное от оригинала.\n"
		"После завершения работы вы можете опубликовать неопубликованное сохранение или снять с публикации уже опубликованные. Откройте сохранение, выберите «Изменить свойства симуляции» и измените статус публикации. Также можно \btsнять с публикации или удалять сохранения\bw: выберите их в разделе «Мои» в браузере сохранений и нажмите одну из кнопок внизу.\n"
		"Если сохранение младше недели и быстро набирает популярность, оно автоматически попадёт на \btглавную страницу\bw. Туда могут попасть только опубликованные сохранения. Модераторы также могут вручную продвинуть сохранение на главную, но это случается редко. Они также могут убрать с главной сохранение, нарушающее правила или неуместное по их мнению.\n"
		"После создания сохранения вы можете пересохранять его сколько угодно раз. Сохраняется короткая \btистория сохранений\bw — щёлкните правой кнопкой на сохранение в браузере и выберите «Просмотр истории». Это полезно, если вы случайно сохранили не то и хотите вернуться к старой версии.\n"
		;

	new InformationMessage("О публикации", info, true);
}

void ServerSaveActivity::ShowRules()
{
	String rules =
		"\boРаздел S: Правила сообщества и общения\n"
		"\bwПри взаимодействии с сообществом следуйте этим правилам. Их соблюдение контролируется администрацией; нарушения можно сообщить через пользователей. Раздел применяется к загруженным сохранениям, комментариям, форуму и другим областям сообщества.\n"
		"\n"
		"\bt1. Соблюдайте нормы грамматики.\bw Официальный язык сообщества — английский, но в региональных и культурных группах его использование не обязательно. Если вы не говорите по-английски хорошо, рекомендуем Google Translate.\n"
		"\bt2. Не спамьте.\bw Универсального определения нет, но идея обычно очевидна. Также считается спамом и может быть скрыто или удалено:\n"
		   "- Создание нескольких тем на одну тему. Объединяйте отзывы и предложения об игре в одну тему.\n"
		   "- Ответ в старой теме ( «necro» или «necroing»). Содержание может быть устаревшим. Рекомендуем создать новую тему для актуального ответа.\n"
		   "- Короткие ответы «+1» и подобные. Не нужно постоянно поднимать тему. Ответы — для содержательной дискуссии, кнопка «+1» — для поддержки.\n"
		   "- Комментарии, что очень длинные или бессмысленные: повтор одной буквы, отсутствие цели. Комментарии на другом языке — исключение.\n"
		   "- Избыточное форматирование. ЗАГЛАВНЫЕ, жирный и курсив уместны в умеренной дозе, не используйте их во всём сообщении.\n"
		"\bt3. Минимизируйте ругательства.\bw Комментарии и сохранения с ругательствами могут быть удалены. Это также включает ругательства на других языках.\n"
		"\bt4. Не загружайте материалы сексуального, оскорбительного или иного неподобающего содержания.\bw\n"
		   "- Включая, но не ограничиваясь: секс, наркотики, расизм, избыточная политика или всё, что оскорбляет или унижает группу людей.\n"
		   "- Упоминания этих тем на других языках также запрещены. Не пытайтесь обойти это правило.\n"
		   "- Запрещены URL и изображения, нарушающие это правило. Включая ссылки и текст в профиле.\n"
		"\bt5. Не рекламируйте сторонние игры, сайты и места, не связанные с The Powder Toy.\bw\n"
		   "- Правило в основном предотвращает рекламу собственных игр и продуктов.\n"
		   "- Неофициальные места сбора сообщества, например Discord, запрещены.\n"
		"\bt6. Троллинг не допускается.\bw Как и в других правилах, чёткого определения нет. Пользователи, систематически троллящие, с большей вероятностью получат бан и более длительный.\n"
		"\bt7. Не выдавайте себя за других.\bw Регистрация аккаунтов с именами, намеренно похожими на других пользователей нашего или других онлайн-сообществ, запрещена.\n"
		"\bt8. Не обсуждайте действия модераторов.\bw При проблемах с баном или удалением контента свяжитесь с модератором через систему сообщений. Обсуждение действий модераторов в остальных случаях нежелательно.\n"
		"\bt9. Не занимайтесь «заднего сиденья» модерированием.\bw Модераторы принимают решения. Пользователям не следует угрожать банами или возможными санкциями. При сомнениях сообщите о проблеме через кнопку «Пожаловаться» или систему сообщений на сайте.\n"
		"\bt10. Пропаганда нарушения общепризнанных законов запрещена.\bw Юрисдикция неясна, но есть общие нормы, включая, но не ограничиваясь:\n"
		   "- Пиратство программ, музыки и т. п.\n"
		   "- Взлом / кража аккаунтов\n"
		   "- Кража / мошенничество\n"
		"\bt11. Не выслеживайте и не травите пользователей.\bw В последние годы это растёт разными способами, в частности:\n"
		   "- «Доксинг» — поиск адреса или реальной личности\n"
		   "- Постоянные сообщения пользователю, когда он просит не контактировать\n"
		   "- Массовое понижение оценок сохранений\n"
		   "- Грубые или необоснованные комментарии к контенту (сохранения, темы форума и т. п.)\n"
		   "- Сговор группы пользователей для «нападения» на кого-то\n"
		   "- Личные споры и ненависть: в комментариях или «хейт-сохранения»\n"
		   "- Дискриминация людей по религии, этническому происхождению и т. п.\n"
		"\n"
		"\boРаздел G: Правила внутри игры\n"
		"\bwЭтот раздел посвящён действиям в игре. Раздел S также применяется в игре, но следующие правила более специфичны для внутриигрового взаимодействия.\n"
		"\bt1. Не присваивайте работу других.\bw Это перезагрузка сохранений других пользователей или использование больших фрагментов. Производные работы допустимы при правильном использовании. При использовании чужой работы по умолчанию нужно указать автора. Если автор явно не указал другие условия — это стандартная политика. Производные работы — с инновационным использованием и процентом оригинальности (сколько своего vs чужого?). Украденные сохранения будут сняты с публикации или отключены.\n"
		"\bt2. Самоголосование и фальшивые голоса запрещены.\bw Создание нескольких аккаунтов для голосования за свои или чужие сохранения. Правило строгое, успешные апелляции редки. Убедитесь, что вы и другие аккаунты не голосуют из одного дома. Все альтернативные аккаунты забанены постоянно, основной — временно, затронутые сохранения отключены.\n"
		"\bt3. Просьбы о голосах нежелательны.\bw Такие сохранения снимаются с публикации до исправления. Примеры:\n"
		   "- Знаки, намекающие на голос «вверх» или «вниз». Знаменитая зелёная стрелка или просьбы о голосах.\n"
		   "- Уловки с голосами: «100 голосов — сделаю лучшую версию». Это фарм голосов, любой фарм запрещён.\n"
		   "- Просьбы о голосах в обмен на использование сохранения или по любой другой причине.\n"
		"\bt4. Не спамьте.\bw Как выше, нет единого стандарта. Примеры спама:\n"
		   "- Загрузка или перезагрузка похожих сохранений в короткий срок. Не обходите систему для видимости и голосов. Включая «пустые» сохранения без цели — они снимаются с публикации.\n"
		   "- Сохранения только с текстом: объявления, поиск помощи. Для этого есть форум и комментарии. Такие сохранения убираются с главной.\n"
		   "- Арт-сохранения не строго запрещены, но могут быть убраны с главной. Мы ценим разнообразие элементов в креативном использовании. Без этого (например, только декор) — обычно убирают с главной.\n"
		"\bt5. Не загружайте материалы сексуального или иного неподобающего содержания. Такие сохранения удаляются, возможен бан.\bw\n"
		   "- Включая, но не ограничиваясь: секс, наркотики, расизм, избыточная политика или всё, что оскорбляет группу людей.\n"
		   "- Не пытайтесь обойти правило. Прямые и косвенные отсылки к этим темам попадают под правило.\n"
		   "- Упоминания на других языках также запрещены.\n"
		   "- Запрещены URL и изображения, нарушающие правило, включая профиль.\n"
		"\bt6. Построение изображений строго запрещено.\bw Скрипты или сторонние инструменты для построения сохранений. Сохранения с CGI удаляются, возможен бан.\n"
		"\bt7. Логотипы и знаки — в минимуме.\bw Такие сохранения могут убрать с главной. Ограничения:\n"
		   "- Избыточное количество логотипов\n"
		   "- Знаки без цели\n"
		   "- Фальшивые знаки обновлений или уведомлений\n"
		   "- Ссылки на другие сохранения без связи с содержанием\n"
		"\bt8. Не ставьте нерелевантные или неподобающие теги.\bw Теги улучшают поиск. Обычно одно слово — описание сохранения. Предложения и субъективные теги могут удалить. Неподобающие теги — риск бана.\n"
		"\bt9. Сохранения, намеренно вызывающие лаг или краши, запрещены.\bw Если большинство пишет о крашах или лаге — правило применяется. Убирают с главной или отключают.\n"
		"\bt10. Не злоупотребляйте системой жалоб.\bw Причины «плохое сохранение» или бессмыслица отнимают время. Если нет нарушения правил или проблемы сообщества — не жалуйтесь. Если считаете, что есть нарушение — жалуйтесь! Бан не будет, если жалоба добросовестная.\n"
		"\bt11. Не просите убрать сохранения с главной.\bw Если нет нарушения правил — оно остаётся. Исключений для арт-сохранений нет, не жалуйтесь на арт.\n"
		"\n"
		"\boРаздел R: Прочее\n"
		"\bwМодераторы могут интерпретировать правила по своему усмотрению. Не все правила равны, некоторые строже. Модераторы решают, что нарушает правила; мы постарались описать всё нежелательное. Обновления правил будут в этой теме.\n"
		"\n"
		"Нарушение может привести к удалению постов и комментариев, снятию с публикации или отключению сохранений, убиранию с главной, а в крайних случаях — временному или постоянному бану. Есть ручные и автоматические меры. Строгость и решения могут различаться между модераторами.\n"
		"\n"
		"При вопросах о том, что разрешено и что нет, свяжитесь с модератором.";

	new InformationMessage("Правила загрузки сохранений", rules, true);
}

void ServerSaveActivity::CheckName(String newname)
{
	auto user = Client::Ref().GetAuthUser();
	if (newname.length() && newname == save->GetName() && user && save->GetUserName() == user->Username)
		titleLabel->SetText("Изменить свойства симуляции:");
	else
		titleLabel->SetText("Загрузить новую симуляцию:");
}

void ServerSaveActivity::OnTick()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Poll();
		if (thumbnailRenderer->GetDone())
		{
			thumbnail = thumbnailRenderer->Finish();
			thumbnailRenderer = nullptr;
		}
	}

	if (uploadSaveRequest && uploadSaveRequest->CheckDone())
	{
		okayButton->Enabled = true;
		try
		{
			save->SetID(uploadSaveRequest->Finish());
			Exit();
			new SaveIDMessage(save->GetID());
			if (onUploaded)
			{
				onUploaded(std::move(save));
			}
		}
		catch (const http::RequestError &ex)
		{
			new ErrorMessage("Ошибка", "Загрузка не удалась с ошибкой:\n" + ByteString(ex.what()).FromUtf8());
		}
		uploadSaveRequest.reset();
	}

	if(saveUploadTask)
		saveUploadTask->Poll();
}

void ServerSaveActivity::OnDraw()
{
	Graphics * g = GetGraphics();
	g->BlendRGBAImage(saveToServerImage->data(), RectSized(Vec2(-10, 0), saveToServerImage->Size()));
	g->DrawFilledRect(RectSized(Position, Size).Inset(-1), 0x000000_rgb);
	g->DrawRect(RectSized(Position, Size), 0xFFFFFF_rgb);

	if (Size.X > 220)
		g->DrawLine(Position + Vec2(Size.X / 2 - 1, 0), Position + Vec2(Size.X / 2 - 1, Size.Y - 1), 0xFFFFFF_rgb);

	if (thumbnail)
	{
		auto rect = RectSized(Position + Vec2(Size.X / 2 + (Size.X / 2 - thumbnail->Size().X) / 2, 25), thumbnail->Size());
		g->BlendImage(thumbnail->Data(), 0xFF, rect);
		g->DrawRect(rect, 0xB4B4B4_rgb);
	}
}

ServerSaveActivity::~ServerSaveActivity()
{
	if (thumbnailRenderer)
	{
		thumbnailRenderer->Abandon();
	}
	delete saveUploadTask;
}
