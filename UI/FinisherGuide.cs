namespace LabPlus;

internal static class FinisherGuide
{
    internal const string Title = "Finisher Visualizer";
    internal const string Summary = "Higher priority wins first; equal priorities use the closest animation start. Move or change your aim to explore the available finishers. Selecting an animation here does not force it in the game.";
    internal const string ChainsawNote = "Chainsaw shows position zones only, without body-part colouring. Zombie Tier 1 front 01 / 02 share a zone; the clip is chosen randomly at activation.";
    internal const string RuneNote = "Use Glory Kill preview with Seek and Destroy unequipped. The rune's extended range is not supported.";
    internal static string Read(string language)
    {
        using var stream = typeof(FinisherGuide).Assembly.GetManifestResourceStream($"LabPlus.FinisherGuide.{language}.txt")
            ?? throw new InvalidOperationException("Finisher guide is missing.");
        using var reader = new StreamReader(stream);return reader.ReadToEnd();
    }
}
